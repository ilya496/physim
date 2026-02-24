#pragma once
/**
 * PhysicsWorld.h — Engine-Grade Rigid-Body Physics
 *
 * FIXES IN THIS VERSION
 * ─────────────────────
 * [CRITICAL] Warm-start applied on EVERY sub-step from stale frame cache:
 *   Previous code called Cache.WarmStart inside SubStep(), which ran 16× per
 *   frame. Each sub-step re-applied 80% of the PREVIOUS FRAME's impulses on
 *   top of whatever the solver already computed. 16 × 0.8 × big_impact_impulse
 *   = catastrophic energy injection. Fix: warm-start only on sub-step 0.
 *   Sub-steps 1–N inherit velocity state from the previous sub-step's solver,
 *   which is exactly what they should be doing.
 *
 * [CRITICAL] Split-impulse MAX_COR = 0.02 m caused tunneling:
 *   2 cm max correction per sub-step at 16 sub-steps = 32 cm/frame max. A box
 *   dropped from rest at 1 m height hits the ground at ~4.4 m/s; first-contact
 *   penetration can easily exceed 2 cm in one sub-step. Fix: raise to 0.2 m and
 *   run 3 passes of position correction per sub-step. Also use LOCAL contact
 *   coords to recompute lever arms after position has moved (removes the stale-
 *   WorldPoint lever-arm error that grows with substep size).
 *
 * [STABILITY] Rotated-box glitch on landing:
 *   Root cause was the warm-start bug above. Secondary cause: when a box lands
 *   slightly tilted, one or two contact points have depth≈0 and get discarded,
 *   leaving a 1-point contact that has high angular compliance → spin jitter.
 *   Fix: keep contacts with depth >= -SLOP (was -0.005) so borderline points
 *   are retained and clamp their depth to 0 (they contribute to preventing
 *   separation without injecting energy).
 *
 * [STABILITY] DistanceJoint was still accumulating lambda across sub-steps:
 *   BeginStep() / ResetAccumulators() only called once per frame. Fix:
 *   reset in SubStep so the joint re-converges from fresh each sub-step.
 */

#include <vector>
#include <algorithm>
#include <array>
#include <map>
#include <unordered_map>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cfloat>
#include <cstdint>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "AABB.h"

 // ============================================================
 //  Shapes
 // ============================================================
enum class ShapeType { Sphere, Box, TriangleMesh };

struct Shape {
    ShapeType Type;
    virtual ~Shape() = default;
    virtual AABB      ComputeLocalAABB()               const = 0;
    virtual glm::mat3 ComputeInertiaTensor(float mass)  const = 0;
    virtual glm::vec3 GetLocalSupport(const glm::vec3& dir) const = 0;
};

struct SphereShape : public Shape {
    float Radius;
    explicit SphereShape(float r) : Radius(r) { Type = ShapeType::Sphere; }
    AABB ComputeLocalAABB() const override { return { glm::vec3(-Radius), glm::vec3(Radius) }; }
    glm::mat3 ComputeInertiaTensor(float mass) const override {
        float I = (2.0f / 5.0f) * mass * Radius * Radius;
        return glm::mat3(I);
    }
    glm::vec3 GetLocalSupport(const glm::vec3& dir) const override {
        float len = glm::length(dir);
        return (len > 1e-8f) ? (dir / len) * Radius : glm::vec3(0, Radius, 0);
    }
};

struct BoxShape : public Shape {
    glm::vec3 HalfExtents;
    explicit BoxShape(const glm::vec3& half) : HalfExtents(half) { Type = ShapeType::Box; }
    AABB ComputeLocalAABB() const override { return { -HalfExtents, HalfExtents }; }
    glm::mat3 ComputeInertiaTensor(float mass) const override {
        float ex = 2.0f * HalfExtents.x, ey = 2.0f * HalfExtents.y, ez = 2.0f * HalfExtents.z;
        float ix = (1.0f / 12.0f) * mass * (ey * ey + ez * ez);
        float iy = (1.0f / 12.0f) * mass * (ex * ex + ez * ez);
        float iz = (1.0f / 12.0f) * mass * (ex * ex + ey * ey);
        return { glm::vec3(ix, 0, 0), glm::vec3(0, iy, 0), glm::vec3(0, 0, iz) };
    }
    glm::vec3 GetLocalSupport(const glm::vec3& dir) const override {
        return {
            dir.x >= 0 ? HalfExtents.x : -HalfExtents.x,
            dir.y >= 0 ? HalfExtents.y : -HalfExtents.y,
            dir.z >= 0 ? HalfExtents.z : -HalfExtents.z
        };
    }
};

// ============================================================
//  Material
// ============================================================
struct PhysicsMaterial {
    float Restitution = 0.2f;
    float Friction = 0.5f;
};
inline float CombineRestitution(const PhysicsMaterial& a, const PhysicsMaterial& b) { return std::max(a.Restitution, b.Restitution); }
inline float CombineFriction(const PhysicsMaterial& a, const PhysicsMaterial& b) { return std::sqrt(a.Friction * b.Friction); }

// ============================================================
//  RigidBody
// ============================================================
enum class BodyType { Static, Kinematic, Dynamic };

struct RigidBody {
    uint32_t  ID = 0;
    BodyType  Type = BodyType::Dynamic;
    void* UserData = nullptr;

    glm::vec3 Position{ 0.0f };
    glm::quat Orientation{ 1, 0, 0, 0 };
    glm::vec3 LinearVelocity{ 0.0f };
    glm::vec3 AngularVelocity{ 0.0f };

    float     Mass = 1.0f;
    float     InverseMass = 1.0f;
    glm::mat3 InverseInertiaLocal{ 1.0f };
    glm::mat3 InverseInertiaWorld{ 1.0f };

    float LinearDamping = 0.02f;
    float AngularDamping = 0.05f;

    glm::vec3 ForceAccumulator{ 0.0f };
    glm::vec3 TorqueAccumulator{ 0.0f };

    PhysicsMaterial Material;
    Shape* CollisionShape = nullptr;
    AABB   WorldAABB;
    float  GravityScale = 1.0f;
    bool   IsAwake = true;
    float  SleepTimer = 0.0f;

    void SetStatic() {
        Type = BodyType::Static;
        InverseMass = 0.0f; InverseInertiaLocal = glm::mat3(0.0f); InverseInertiaWorld = glm::mat3(0.0f);
        LinearVelocity = {}; AngularVelocity = {}; IsAwake = false;
    }
    void RecalculateMassProperties() {
        if (Type == BodyType::Static || !CollisionShape || Mass <= 0.0f) { SetStatic(); return; }
        InverseMass = 1.0f / Mass;
        glm::mat3 I = CollisionShape->ComputeInertiaTensor(Mass);
        InverseInertiaLocal = (std::abs(glm::determinant(I)) > 1e-12f) ? glm::inverse(I) : glm::mat3(0.0f);
    }
    void UpdateWorldInertia() {
        if (Type != BodyType::Dynamic) return;
        glm::mat3 R = glm::mat3_cast(Orientation);
        InverseInertiaWorld = R * InverseInertiaLocal * glm::transpose(R);
    }
    void UpdateAABB(float margin = 0.01f) {
        if (!CollisionShape) return;
        AABB lo = CollisionShape->ComputeLocalAABB();
        glm::vec3 lc = (lo.Min + lo.Max) * 0.5f, le = (lo.Max - lo.Min) * 0.5f;
        glm::vec3 wc = Position + (Orientation * lc);
        glm::mat3 R = glm::mat3_cast(Orientation);
        glm::vec3 we = glm::abs(R[0]) * le.x + glm::abs(R[1]) * le.y + glm::abs(R[2]) * le.z;
        WorldAABB.Min = wc - we - glm::vec3(margin);
        WorldAABB.Max = wc + we + glm::vec3(margin);
    }
    void WakeUp() { if (!IsAwake) { IsAwake = true; SleepTimer = 0.0f; } }

    glm::vec3 LocalToWorld(const glm::vec3& lp) const { return Position + (Orientation * lp); }
    glm::vec3 WorldToLocal(const glm::vec3& wp) const { return glm::conjugate(Orientation) * (wp - Position); }
    glm::vec3 VelocityAt(const glm::vec3& wp) const { return LinearVelocity + glm::cross(AngularVelocity, wp - Position); }

    void ApplyForce(const glm::vec3& f) { if (IsDynamic()) { ForceAccumulator += f; WakeUp(); } }
    void ApplyTorque(const glm::vec3& t) { if (IsDynamic()) { TorqueAccumulator += t; WakeUp(); } }
    void ApplyForceAtPoint(const glm::vec3& f, const glm::vec3& wp) {
        if (!IsDynamic()) return; ForceAccumulator += f; TorqueAccumulator += glm::cross(wp - Position, f); WakeUp();
    }
    void ApplyCentralImpulse(const glm::vec3& j) { if (IsDynamic()) LinearVelocity += j * InverseMass; }
    void ApplyImpulse(const glm::vec3& j, const glm::vec3& r) {
        if (!IsDynamic()) return; LinearVelocity += j * InverseMass; AngularVelocity += InverseInertiaWorld * glm::cross(r, j);
    }

    bool IsStatic()   const { return Type == BodyType::Static; }
    bool IsKinematic()const { return Type == BodyType::Kinematic; }
    bool IsDynamic()  const { return Type == BodyType::Dynamic; }
    bool CanMove()    const { return Type == BodyType::Dynamic; }
};

// ============================================================
//  Contact / Manifold
// ============================================================
struct ContactPoint {
    glm::vec3 WorldPointA, WorldPointB;
    glm::vec3 LocalPointA, LocalPointB;    // Used for warm-start matching + position correction
    float     Depth = 0.0f;

    float NormalImpulse = 0.0f;
    float TangentImpulse0 = 0.0f;
    float TangentImpulse1 = 0.0f;

    glm::vec3 Tangent0{ 1, 0, 0 };
    glm::vec3 Tangent1{ 0, 0, 1 };
};

struct Manifold {
    RigidBody* BodyA = nullptr;
    RigidBody* BodyB = nullptr;
    glm::vec3  Normal{ 0, 1, 0 };
    std::vector<ContactPoint> Contacts;

    uint64_t Key() const {
        uint32_t a = BodyA ? BodyA->ID : 0u, b = BodyB ? BodyB->ID : 0u;
        if (a > b) std::swap(a, b);
        return (uint64_t(a) << 32) | uint64_t(b);
    }
};

// ============================================================
//  Manifold cache  —  warm-start on first sub-step ONLY
// ============================================================
class ManifoldCache {
    struct Cached { glm::vec3 LA, LB; float NI, T0, T1; };
    std::unordered_map<uint64_t, std::vector<Cached>> C;
    static constexpr float MATCH_SQ = 0.09f;   // 0.3 m match radius in local space
    static constexpr float WARM_SCALE = 0.85f;   // slight decay avoids re-applying stale impulses

public:
    void WarmStart(Manifold& m) const {
        auto it = C.find(m.Key());
        if (it == C.end()) return;
        for (auto& c : m.Contacts) {
            float best = MATCH_SQ; const Cached* pick = nullptr;
            for (const auto& cc : it->second) {
                float d = glm::length2(c.LocalPointA - cc.LA) + glm::length2(c.LocalPointB - cc.LB);
                if (d < best) { best = d; pick = &cc; }
            }
            if (pick) {
                c.NormalImpulse = pick->NI * WARM_SCALE;
                c.TangentImpulse0 = pick->T0 * WARM_SCALE;
                c.TangentImpulse1 = pick->T1 * WARM_SCALE;
            }
        }
    }
    void Store(const Manifold& m) {
        std::vector<Cached> v; v.reserve(m.Contacts.size());
        for (const auto& c : m.Contacts)
            v.push_back({ c.LocalPointA, c.LocalPointB, c.NormalImpulse, c.TangentImpulse0, c.TangentImpulse1 });
        C[m.Key()] = std::move(v);
    }
    void Clear() { C.clear(); }
};

// ============================================================
//  Math helpers
// ============================================================
inline void BuildTangentBasis(const glm::vec3& n, glm::vec3& t0, glm::vec3& t1) {
    t0 = (std::abs(n.x) >= 0.57735f)
        ? glm::normalize(glm::vec3(n.y, -n.x, 0.0f))
        : glm::normalize(glm::vec3(0.0f, n.z, -n.y));
    t1 = glm::cross(n, t0);
}

inline float EffectiveMass(RigidBody* A, RigidBody* B,
    const glm::vec3& rA, const glm::vec3& rB, const glm::vec3& ax)
{
    glm::vec3 rAxN = glm::cross(rA, ax), rBxN = glm::cross(rB, ax);
    return A->InverseMass + B->InverseMass
        + glm::dot(rAxN, A->InverseInertiaWorld * rAxN)
        + glm::dot(rBxN, B->InverseInertiaWorld * rBxN);
}

// ============================================================
//  3-D Sutherland-Hodgman clipping
// ============================================================
static std::vector<glm::vec3> ClipByPlane(const std::vector<glm::vec3>& poly,
    const glm::vec3& n, float d)
{
    std::vector<glm::vec3> out;
    size_t sz = poly.size();
    for (size_t i = 0; i < sz; ++i) {
        const glm::vec3& pA = poly[i], & pB = poly[(i + 1) % sz];
        float dA = glm::dot(n, pA) - d, dB = glm::dot(n, pB) - d;
        bool aIn = dA >= 0.0f, bIn = dB >= 0.0f;
        if (aIn) out.push_back(pA);
        if (aIn != bIn) { float t = dA / (dA - dB); out.push_back(pA + t * (pB - pA)); }
    }
    return out;
}

static std::vector<glm::vec3> ClipToRefFace(const std::array<glm::vec3, 4>& iv,
    const glm::vec3& fc, const glm::vec3& U, const glm::vec3& V, float hU, float hV)
{
    std::vector<glm::vec3> p(iv.begin(), iv.end());
    struct Pl { glm::vec3 n; float d; };
    Pl planes[4] = {
        {  U,  glm::dot(U, fc) - hU },
        { -U,  glm::dot(-U, fc) - hU },
        {  V,  glm::dot(V, fc) - hV },
        { -V,  glm::dot(-V, fc) - hV },
    };
    for (const auto& pl : planes) { p = ClipByPlane(p, pl.n, pl.d); if (p.empty()) return p; }
    return p;
}

// ============================================================
//  Box face builder
// ============================================================
static void GetBoxFace(RigidBody* body, BoxShape* box, int axisIdx, int sign,
    std::array<glm::vec3, 4>& verts, glm::vec3& center, glm::vec3& normal,
    glm::vec3& U, glm::vec3& V, float& hU, float& hV)
{
    glm::mat3 R = glm::mat3_cast(body->Orientation);
    int a = axisIdx, b = (a + 1) % 3, c = (a + 2) % 3;
    normal = R[a] * float(sign);
    center = body->Position + normal * box->HalfExtents[a];
    U = R[b]; V = R[c]; hU = box->HalfExtents[b]; hV = box->HalfExtents[c];
    if (glm::dot(glm::cross(U, V), normal) < 0.0f) U = -U;
    verts[0] = center + U * hU + V * hV;
    verts[1] = center + U * hU - V * hV;
    verts[2] = center - U * hU - V * hV;
    verts[3] = center - U * hU + V * hV;
}

// ============================================================
//  SAT overlap (face axes only)
// ============================================================
static float SATOverlap(const glm::vec3& axis, BoxShape* bA, RigidBody* rA, BoxShape* bB, RigidBody* rB) {
    glm::mat3 RA = glm::mat3_cast(rA->Orientation), RB = glm::mat3_cast(rB->Orientation);
    float pA = bA->HalfExtents.x * std::abs(glm::dot(RA[0], axis))
        + bA->HalfExtents.y * std::abs(glm::dot(RA[1], axis))
        + bA->HalfExtents.z * std::abs(glm::dot(RA[2], axis));
    float pB = bB->HalfExtents.x * std::abs(glm::dot(RB[0], axis))
        + bB->HalfExtents.y * std::abs(glm::dot(RB[1], axis))
        + bB->HalfExtents.z * std::abs(glm::dot(RB[2], axis));
    return (pA + pB) - std::abs(glm::dot(rB->Position - rA->Position, axis));
}

// ============================================================
//  Contact reduction (≤ 4 points)
// ============================================================
static void ReduceContacts(std::vector<ContactPoint>& pts) {
    if (pts.size() <= 4) return;
    auto it = std::max_element(pts.begin(), pts.end(),
        [](const ContactPoint& a, const ContactPoint& b) { return a.Depth < b.Depth; });
    std::vector<ContactPoint> r; r.push_back(*it); pts.erase(it);
    while (r.size() < 4 && !pts.empty()) {
        float best = -1.0f; auto bi = pts.begin();
        for (auto jt = pts.begin(); jt != pts.end(); ++jt) {
            float minD = FLT_MAX;
            for (const auto& x : r) minD = std::min(minD, glm::length2(jt->WorldPointA - x.WorldPointA));
            if (minD > best) { best = minD; bi = jt; }
        }
        r.push_back(*bi); pts.erase(bi);
    }
    pts = std::move(r);
}

// ============================================================
//  Collision tests
// ============================================================
inline bool TestSphereSphere(RigidBody* A, RigidBody* B, Manifold& m) {
    auto* sA = static_cast<SphereShape*>(A->CollisionShape);
    auto* sB = static_cast<SphereShape*>(B->CollisionShape);
    glm::vec3 d = B->Position - A->Position;
    float d2 = glm::length2(d), rs = sA->Radius + sB->Radius;
    if (d2 > rs * rs) return false;
    float dist = std::sqrt(d2);
    glm::vec3 n = dist > 1e-8f ? d / dist : glm::vec3(0, 1, 0);
    m.BodyA = A; m.BodyB = B; m.Normal = n;
    ContactPoint c;
    c.Depth = rs - dist;
    c.WorldPointA = A->Position + n * sA->Radius;
    c.WorldPointB = B->Position - n * sB->Radius;
    c.LocalPointA = A->WorldToLocal(c.WorldPointA);
    c.LocalPointB = B->WorldToLocal(c.WorldPointB);
    m.Contacts.push_back(c); return true;
}

inline bool TestSphereBox(RigidBody* sph, RigidBody* box, Manifold& m) {
    auto* S = static_cast<SphereShape*>(sph->CollisionShape);
    auto* B = static_cast<BoxShape*>  (box->CollisionShape);
    glm::vec3 lc = box->WorldToLocal(sph->Position);
    glm::vec3 cl = glm::clamp(lc, -B->HalfExtents, B->HalfExtents);
    glm::vec3 df = lc - cl;
    float d2 = glm::length2(df);
    if (d2 > S->Radius * S->Radius) return false;
    m.BodyA = sph; m.BodyB = box; m.Contacts.clear();
    ContactPoint c;
    if (d2 > 1e-8f) {
        float dist = std::sqrt(d2);
        m.Normal = -glm::normalize(box->Orientation * (df / dist));
        c.Depth = S->Radius - dist;
        c.WorldPointA = sph->Position + m.Normal * S->Radius;
        c.WorldPointB = box->LocalToWorld(cl);
    }
    else {
        glm::vec3 d = B->HalfExtents - glm::abs(lc);
        int ax = 0; if (d.y < d.x) ax = 1; if (d.z < d[ax]) ax = 2;
        glm::vec3 fn(0.0f); fn[ax] = lc[ax] >= 0.0f ? 1.0f : -1.0f;
        m.Normal = -glm::normalize(box->Orientation * fn);
        c.Depth = S->Radius + d[ax];
        c.WorldPointA = sph->Position - m.Normal * S->Radius;
        c.WorldPointB = box->LocalToWorld(fn * B->HalfExtents[ax]);
    }
    c.LocalPointA = sph->WorldToLocal(c.WorldPointA);
    c.LocalPointB = box->WorldToLocal(c.WorldPointB);
    m.Contacts.push_back(c); return true;
}

// Box-Box: face axes only (no edge-edge).
// Edge-edge axes cause:
//   (a) single-contact normals that flicker every frame → jitter
//   (b) boxes "balancing" stably on a single edge (non-physical)
// Face-only contacts always produce stable 1–4 point manifolds.
inline bool TestBoxBox(RigidBody* A, RigidBody* B, Manifold& m) {
    auto* bA = static_cast<BoxShape*>(A->CollisionShape);
    auto* bB = static_cast<BoxShape*>(B->CollisionShape);
    glm::mat3 RA = glm::mat3_cast(A->Orientation), RB = glm::mat3_cast(B->Orientation);

    float minOv = FLT_MAX;
    int   bestIdx = 0;
    bool  refIsA = true;

    // Test 6 face axes, pick minimum overlap
    auto testFace = [&](const glm::vec3& ax, bool fromA, int idx) -> bool {
        float ov = SATOverlap(ax, bA, A, bB, B);
        if (ov < 0.0f) return false;
        if (ov < minOv) { minOv = ov; refIsA = fromA; bestIdx = idx; }
        return true;
        };
    for (int i = 0; i < 3; ++i) if (!testFace(RA[i], true, i)) return false;
    for (int i = 0; i < 3; ++i) if (!testFace(RB[i], false, i)) return false;

    // Snap normal to the exact rotation-matrix column — eliminates floating-point
    // wobble that otherwise causes the axis to flicker between two near-equal faces.
    RigidBody* refBody = refIsA ? A : B;   BoxShape* refBox = refIsA ? bA : bB;
    RigidBody* incBody = refIsA ? B : A;   BoxShape* incBox = refIsA ? bB : bA;
    glm::mat3  refR = glm::mat3_cast(refBody->Orientation);
    glm::mat3  incR = glm::mat3_cast(incBody->Orientation);
    glm::vec3  AB = B->Position - A->Position;

    glm::vec3 bestAx = refR[bestIdx];
    if (glm::dot(bestAx, AB) < 0.0f) bestAx = -bestAx;

    m.BodyA = A; m.BodyB = B; m.Normal = bestAx; m.Contacts.clear();

    // Reference face
    int refSign = (glm::dot(refR[bestIdx], bestAx) >= 0.0f) ? +1 : -1;
    std::array<glm::vec3, 4> refV;
    glm::vec3 refC, refN, refU, refVv; float hU, hV;
    GetBoxFace(refBody, refBox, bestIdx, refSign, refV, refC, refN, refU, refVv, hU, hV);

    // Incident face: most anti-parallel to refNormal
    int   incAx = 0;
    float minDot = FLT_MAX;
    for (int i = 0; i < 3; ++i) {
        float d = glm::dot(incR[i], refN);
        if (d < minDot) { minDot = d; incAx = i; }
    }
    int incSign = (glm::dot(incR[incAx], refN) >= 0.0f) ? -1 : +1;
    std::array<glm::vec3, 4> incV;
    glm::vec3 incC, incN, incU, incVv; float iHU, iHV;
    GetBoxFace(incBody, incBox, incAx, incSign, incV, incC, incN, incU, incVv, iHU, iHV);

    // Clip incident polygon to reference face side-planes
    auto clipped = ClipToRefFace(incV, refC, refU, refVv, hU, hV);
    if (clipped.empty()) return false;

    float refD = glm::dot(refN, refC);

    // Relaxed depth threshold: keep contacts with depth >= -SLOP.
    // Old code used -0.005 which discarded edge-contact points on rotated boxes
    // that legitimately lie right at the surface, leaving a 1-point manifold
    // with poor rotational stability. These near-zero depth points prevent
    // separation without injecting energy (NormalImpulse clamped to >= 0).
    const float KEEP_THRESHOLD = -0.01f;

    for (const auto& p : clipped) {
        float depth = refD - glm::dot(refN, p);
        if (depth < KEEP_THRESHOLD) continue;        // truly in front of surface → discard
        ContactPoint c;
        c.Depth = std::max(depth, 0.0f);             // clamp; near-zero points contribute legally
        glm::vec3 onRef = p + refN * depth;
        c.WorldPointA = refIsA ? onRef : p;
        c.WorldPointB = refIsA ? p : onRef;
        c.LocalPointA = A->WorldToLocal(c.WorldPointA);
        c.LocalPointB = B->WorldToLocal(c.WorldPointB);
        m.Contacts.push_back(c);
    }
    if (m.Contacts.empty()) return false;
    ReduceContacts(m.Contacts);
    return true;
}

// ============================================================
//  Dispatcher
// ============================================================
static void DispatchCollision(RigidBody* A, RigidBody* B, std::vector<Manifold>& out) {
    if (!A->CollisionShape || !B->CollisionShape) return;
    if (A->IsStatic() && B->IsStatic()) return;
    if (!A->IsAwake && !B->IsAwake) return;

    ShapeType tA = A->CollisionShape->Type, tB = B->CollisionShape->Type;
    Manifold m; bool hit = false;

    auto flip = [](Manifold& m) {
        m.Normal = -m.Normal; std::swap(m.BodyA, m.BodyB);
        for (auto& c : m.Contacts) { std::swap(c.WorldPointA, c.WorldPointB); std::swap(c.LocalPointA, c.LocalPointB); }
        };

    if (tA == ShapeType::Sphere && tB == ShapeType::Sphere) hit = TestSphereSphere(A, B, m);
    else if (tA == ShapeType::Sphere && tB == ShapeType::Box) hit = TestSphereBox(A, B, m);
    else if (tA == ShapeType::Box && tB == ShapeType::Sphere) { hit = TestSphereBox(B, A, m); if (hit) flip(m); }
    else if (tA == ShapeType::Box && tB == ShapeType::Box) hit = TestBoxBox(A, B, m);

    if (hit && !m.Contacts.empty()) out.push_back(std::move(m));
}

// ============================================================
//  Sort-and-Sweep broadphase  O(N log N)
// ============================================================
class SortAndSweep {
public:
    std::vector<std::pair<uint32_t, uint32_t>> Query(const std::vector<RigidBody*>& bodies) {
        std::vector<std::pair<float, uint32_t>> ev; ev.reserve(bodies.size());
        for (uint32_t i = 0; i < (uint32_t)bodies.size(); ++i)
            ev.push_back({ bodies[i]->WorldAABB.Min.x, i });
        std::sort(ev.begin(), ev.end());

        std::vector<std::pair<uint32_t, uint32_t>> pairs;
        std::vector<uint32_t> active; active.reserve(32);

        for (const auto& [minX, i] : ev) {
            active.erase(std::remove_if(active.begin(), active.end(),
                [&](uint32_t k) { return bodies[k]->WorldAABB.Max.x < minX; }), active.end());
            for (uint32_t j : active) {
                if (bodies[i]->IsStatic() && bodies[j]->IsStatic()) continue;
                if (!bodies[i]->IsAwake && !bodies[j]->IsAwake) continue;
                if (bodies[i]->WorldAABB.Overlaps(bodies[j]->WorldAABB))
                    pairs.emplace_back(std::min(i, j), std::max(i, j));
            }
            active.push_back(i);
        }
        std::sort(pairs.begin(), pairs.end());
        pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
        return pairs;
    }
};

// ============================================================
//  Constraints
// ============================================================
struct Constraint {
    RigidBody* BodyA = nullptr;
    RigidBody* BodyB = nullptr;
    virtual void BeginSubStep() {}                          // called at start of every sub-step
    virtual void SolveVelocity(float dt, float invDt) = 0;
    virtual ~Constraint() = default;
};

// ---- Distance joint ----
struct DistanceJoint : public Constraint {
    glm::vec3 LocalAnchorA{ 0.0f };
    glm::vec3 LocalAnchorB{ 0.0f };
    float TargetLength = 1.0f;
    float ERP = 0.2f;     // error-reduction per second fraction
    float MaxImpulse = 1e6f;     // per sub-step clamp
    float AccumLambda = 0.0f;

    void BeginSubStep() override { AccumLambda = 0.0f; }   // reset each sub-step

    void SolveVelocity(float dt, float invDt) override {
        if (!BodyA || !BodyB) return;
        glm::vec3 wA = BodyA->LocalToWorld(LocalAnchorA);
        glm::vec3 wB = BodyB->LocalToWorld(LocalAnchorB);
        glm::vec3 rA = wA - BodyA->Position, rB = wB - BodyB->Position;
        glm::vec3 df = wB - wA;
        float dist = glm::length(df);
        if (dist < 1e-8f) return;
        glm::vec3 n = df / dist;
        float C = dist - TargetLength;
        float em = EffectiveMass(BodyA, BodyB, rA, rB, n);
        if (em < 1e-10f) return;
        float Jv = glm::dot(BodyB->VelocityAt(wB) - BodyA->VelocityAt(wA), n);
        float bias = ERP * invDt * C;
        float lam = -(Jv + bias) / em;
        lam = std::clamp(lam, -MaxImpulse * dt, MaxImpulse * dt);
        AccumLambda += lam;
        glm::vec3 J = n * lam;
        if (BodyA->CanMove()) BodyA->ApplyImpulse(-J, rA);
        if (BodyB->CanMove()) BodyB->ApplyImpulse(J, rB);
    }
};

// ---- Ball joint (point-to-point, 3 DOF locked) ----
struct BallJoint : public Constraint {
    glm::vec3 LocalAnchorA{ 0.0f };
    glm::vec3 LocalAnchorB{ 0.0f };
    float     ERP = 0.2f;
    float     AccumLam[3] = {};

    void BeginSubStep() override { AccumLam[0] = AccumLam[1] = AccumLam[2] = 0.0f; }

    void SolveVelocity(float /*dt*/, float invDt) override {
        if (!BodyA || !BodyB) return;
        glm::vec3 wA = BodyA->LocalToWorld(LocalAnchorA);
        glm::vec3 wB = BodyB->LocalToWorld(LocalAnchorB);
        glm::vec3 rA = wA - BodyA->Position, rB = wB - BodyB->Position;
        glm::vec3 posErr = wB - wA;
        for (int i = 0; i < 3; ++i) {
            glm::vec3 ax(0.0f); ax[i] = 1.0f;
            float em = EffectiveMass(BodyA, BodyB, rA, rB, ax);
            if (em < 1e-10f) continue;
            float Jv = glm::dot(BodyB->VelocityAt(wB) - BodyA->VelocityAt(wA), ax);
            float bias = ERP * invDt * posErr[i];
            float lam = -(Jv + bias) / em;
            AccumLam[i] += lam;
            glm::vec3 J = ax * lam;
            if (BodyA->CanMove()) BodyA->ApplyImpulse(-J, rA);
            if (BodyB->CanMove()) BodyB->ApplyImpulse(J, rB);
        }
    }
};

// ---- Hinge joint (3 linear + 2 angular DOF locked) ----
struct HingeJoint : public Constraint {
    glm::vec3 LocalAnchorA{ 0.0f };
    glm::vec3 LocalAnchorB{ 0.0f };
    glm::vec3 LocalAxisA{ 0, 1, 0 };
    float     ERP = 0.2f;
    float     AccumLin[3] = {};
    float     AccumAng[2] = {};

    void BeginSubStep() override {
        AccumLin[0] = AccumLin[1] = AccumLin[2] = 0.0f;
        AccumAng[0] = AccumAng[1] = 0.0f;
    }

    void SolveVelocity(float /*dt*/, float invDt) override {
        if (!BodyA || !BodyB) return;
        glm::vec3 wA = BodyA->LocalToWorld(LocalAnchorA);
        glm::vec3 wB = BodyB->LocalToWorld(LocalAnchorB);
        glm::vec3 rA = wA - BodyA->Position, rB = wB - BodyB->Position;
        glm::vec3 posErr = wB - wA;

        // Linear: 3 equality constraints
        for (int i = 0; i < 3; ++i) {
            glm::vec3 ax(0.0f); ax[i] = 1.0f;
            float em = EffectiveMass(BodyA, BodyB, rA, rB, ax);
            if (em < 1e-10f) continue;
            float Jv = glm::dot(BodyB->VelocityAt(wB) - BodyA->VelocityAt(wA), ax);
            float bias = ERP * invDt * posErr[i];
            float lam = -(Jv + bias) / em;
            AccumLin[i] += lam;
            glm::vec3 J = ax * lam;
            if (BodyA->CanMove()) BodyA->ApplyImpulse(-J, rA);
            if (BodyB->CanMove()) BodyB->ApplyImpulse(J, rB);
        }

        // Angular: lock 2 DOF perpendicular to hinge axis
        glm::vec3 worldAxis = BodyA->Orientation * LocalAxisA;
        glm::vec3 t0, t1; BuildTangentBasis(worldAxis, t0, t1);
        for (int i = 0; i < 2; ++i) {
            glm::vec3 tang = (i == 0) ? t0 : t1;
            float invM = glm::dot(tang, (BodyA->InverseInertiaWorld + BodyB->InverseInertiaWorld) * tang);
            if (invM < 1e-10f) continue;
            float Jv = glm::dot(BodyB->AngularVelocity - BodyA->AngularVelocity, tang);
            float lam = -Jv / invM;
            AccumAng[i] += lam;
            glm::vec3 J = tang * lam;
            if (BodyA->CanMove()) BodyA->AngularVelocity -= BodyA->InverseInertiaWorld * J;
            if (BodyB->CanMove()) BodyB->AngularVelocity += BodyB->InverseInertiaWorld * J;
        }
    }
};

// ============================================================
//  PhysicsWorld
// ============================================================
struct BodyState { glm::vec3 Position; glm::quat Orientation; glm::vec3 LinearVelocity; glm::vec3 AngularVelocity; };
using PhysicsSnapshot = std::map<uint32_t, BodyState>;

class PhysicsWorld {
public:
    glm::vec3 Gravity{ 0, -9.81f, 0 };
    int   SolverIterations = 25;
    int   SubSteps = 16;
    int   PositionIterations = 3;       // position correction passes per sub-step
    bool  EnableSleeping = true;
    float SleepTimeThreshold = 0.5f;
    float SleepLinVelThreshold = 0.04f;
    float SleepAngVelThreshold = 0.04f;
    float DefaultLinearDamping = 0.02f;
    float DefaultAngularDamping = 0.05f;

    std::vector<RigidBody*>  Bodies;
    std::vector<Manifold>    Contacts;
    std::vector<Constraint*> Constraints;
    ManifoldCache Cache;
    SortAndSweep  Broadphase;
    uint32_t      NextID = 1;

    ~PhysicsWorld() { for (auto* b : Bodies) delete b; for (auto* c : Constraints) delete c; }

    RigidBody* CreateBody(const glm::vec3& pos, Shape* shape,
        BodyType type = BodyType::Dynamic, float mass = 1.0f)
    {
        RigidBody* b = new RigidBody();
        b->ID = NextID++; b->Position = pos; b->CollisionShape = shape;
        b->Type = type;   b->Mass = mass;
        b->LinearDamping = DefaultLinearDamping;
        b->AngularDamping = DefaultAngularDamping;
        b->RecalculateMassProperties(); b->UpdateWorldInertia(); b->UpdateAABB();
        Bodies.push_back(b); return b;
    }

    void RemoveBody(RigidBody* body) {
        auto it = std::find(Bodies.begin(), Bodies.end(), body);
        if (it != Bodies.end()) { Bodies.erase(it); delete body; }
    }

    DistanceJoint* AddDistanceJoint(RigidBody* a, RigidBody* b,
        const glm::vec3& anA, const glm::vec3& anB, float length = -1.0f)
    {
        auto* j = new DistanceJoint();
        j->BodyA = a; j->BodyB = b; j->LocalAnchorA = anA; j->LocalAnchorB = anB;
        j->TargetLength = (length < 0.0f)
            ? glm::length(a->LocalToWorld(anA) - b->LocalToWorld(anB)) : length;
        Constraints.push_back(j); return j;
    }

    BallJoint* AddBallJoint(RigidBody* a, RigidBody* b, const glm::vec3& worldAnchor) {
        auto* j = new BallJoint();
        j->BodyA = a; j->BodyB = b;
        j->LocalAnchorA = a->WorldToLocal(worldAnchor);
        j->LocalAnchorB = b->WorldToLocal(worldAnchor);
        Constraints.push_back(j); return j;
    }

    HingeJoint* AddHingeJoint(RigidBody* a, RigidBody* b,
        const glm::vec3& worldAnchor, const glm::vec3& worldAxis)
    {
        auto* j = new HingeJoint();
        j->BodyA = a; j->BodyB = b;
        j->LocalAnchorA = a->WorldToLocal(worldAnchor);
        j->LocalAnchorB = b->WorldToLocal(worldAnchor);
        j->LocalAxisA = glm::conjugate(a->Orientation) * glm::normalize(worldAxis);
        Constraints.push_back(j); return j;
    }

    void SetState(const PhysicsSnapshot& snap) {
        for (auto* b : Bodies) {
            auto it = snap.find(b->ID); if (it == snap.end()) continue;
            const auto& s = it->second;
            b->Position = s.Position; b->Orientation = s.Orientation;
            b->LinearVelocity = s.LinearVelocity; b->AngularVelocity = s.AngularVelocity;
            b->UpdateWorldInertia(); b->UpdateAABB();
        }
    }
    PhysicsSnapshot GetState() const {
        PhysicsSnapshot snap;
        for (const auto* b : Bodies)
            snap[b->ID] = { b->Position, b->Orientation, b->LinearVelocity, b->AngularVelocity };
        return snap;
    }

    void Step(float dt) {
        if (dt <= 0.0f) return;
        float subDt = dt / float(SubSteps);

        for (int s = 0; s < SubSteps; ++s) {
            // FIX: warm-start only on the first sub-step.
            //
            // Calling Cache.WarmStart inside every sub-step caused each of 16
            // sub-steps to re-apply 85% of the PREVIOUS FRAME's impulses.
            // For a box that just landed, that previous-frame impulse was large.
            // 16 × 0.85 × large_impact = massive energy injection each frame.
            SubStep(subDt, /*doWarmStart=*/ s == 0);
        }

        // Store cache AFTER all sub-steps so sub-step N+1 never warm-starts
        // from sub-step N's impulses within the same frame.
        for (const auto& man : Contacts) Cache.Store(man);
    }

private:
    void SubStep(float dt, bool doWarmStart) {
        const float invDt = 1.0f / dt;

        // 1. Integrate forces → velocities
        for (auto* b : Bodies) {
            if (!b->IsDynamic() || !b->IsAwake) continue;
            b->LinearVelocity += (b->ForceAccumulator * b->InverseMass + Gravity * b->GravityScale) * dt;
            b->AngularVelocity += (b->InverseInertiaWorld * b->TorqueAccumulator) * dt;
            b->ForceAccumulator = glm::vec3(0.0f);
            b->TorqueAccumulator = glm::vec3(0.0f);
            b->LinearVelocity *= std::exp(-b->LinearDamping * dt);
            b->AngularVelocity *= std::exp(-b->AngularDamping * dt);
        }

        // 2. Broadphase + narrowphase
        Contacts.clear();
        for (auto* b : Bodies) b->UpdateAABB();
        for (const auto& [i, j] : Broadphase.Query(Bodies))
            DispatchCollision(Bodies[i], Bodies[j], Contacts);
        for (auto& man : Contacts) { man.BodyA->WakeUp(); man.BodyB->WakeUp(); }

        // 3. Tangent bases + optional warm-start (first sub-step only)
        for (auto& man : Contacts) {
            for (auto& c : man.Contacts) BuildTangentBasis(man.Normal, c.Tangent0, c.Tangent1);
            if (doWarmStart) {
                Cache.WarmStart(man);
                WarmStartManifold(man);
            }
        }

        // 4. Reset joint accumulators each sub-step
        for (auto* con : Constraints) con->BeginSubStep();

        // 5. Velocity-level PGS
        for (int iter = 0; iter < SolverIterations; ++iter) {
            SolveContactVelocities();
            for (auto* con : Constraints) con->SolveVelocity(dt, invDt);
        }

        // 6. Integrate velocities → positions
        for (auto* b : Bodies) {
            if (!b->IsDynamic() || !b->IsAwake) continue;
            b->Position += b->LinearVelocity * dt;
            float wLen = glm::length(b->AngularVelocity);
            if (wLen > 1e-8f)
                b->Orientation = glm::normalize(glm::angleAxis(wLen * dt, b->AngularVelocity / wLen) * b->Orientation);
        }

        // 7. Split-impulse position correction — multiple passes, lever arms
        //    recomputed from LOCAL coords so they stay accurate after step 6.
        for (int pass = 0; pass < PositionIterations; ++pass)
            SolveContactPositions();

        // 8. Update world inertia + AABB
        for (auto* b : Bodies) if (b->IsDynamic()) {
            b->UpdateWorldInertia(); b->UpdateAABB();
        }

        // 9. Sleep
        if (EnableSleeping) TickSleep(dt);
    }

    void WarmStartManifold(Manifold& man) {
        RigidBody* A = man.BodyA, * B = man.BodyB;
        for (auto& c : man.Contacts) {
            glm::vec3 rA = c.WorldPointA - A->Position, rB = c.WorldPointB - B->Position;
            glm::vec3 P = man.Normal * c.NormalImpulse
                + c.Tangent0 * c.TangentImpulse0
                + c.Tangent1 * c.TangentImpulse1;
            if (A->CanMove()) { A->LinearVelocity -= P * A->InverseMass; A->AngularVelocity -= A->InverseInertiaWorld * glm::cross(rA, P); }
            if (B->CanMove()) { B->LinearVelocity += P * B->InverseMass; B->AngularVelocity += B->InverseInertiaWorld * glm::cross(rB, P); }
        }
    }

    void SolveContactVelocities() {
        // Restitution only when approaching fast enough.
        // velN < -REST_THRESH (negative = approaching).
        // NOT abs(velN) — that was the sky-launch bug: applying restitution
        // when bodies were already SEPARATING fast.
        const float REST_THRESH = 1.5f;

        for (auto& man : Contacts) {
            RigidBody* A = man.BodyA, * B = man.BodyB;
            float e = CombineRestitution(A->Material, B->Material);
            float mu = CombineFriction(A->Material, B->Material);

            for (auto& c : man.Contacts) {
                glm::vec3 rA = c.WorldPointA - A->Position, rB = c.WorldPointB - B->Position;

                // ── Normal impulse ─────────────────────────────────────────
                glm::vec3 vRel = B->VelocityAt(c.WorldPointB) - A->VelocityAt(c.WorldPointA);
                float velN = glm::dot(vRel, man.Normal);
                float em = EffectiveMass(A, B, rA, rB, man.Normal);
                if (em < 1e-10f) continue;

                // No Baumgarte — position errors are handled by SolveContactPositions.
                float coefE = (velN < -REST_THRESH) ? e : 0.0f;
                float jN = -(1.0f + coefE) * velN / em;
                float prev = c.NormalImpulse;
                c.NormalImpulse = std::max(0.0f, prev + jN);
                float dN = c.NormalImpulse - prev;
                glm::vec3 PN = man.Normal * dN;
                if (A->CanMove()) { A->LinearVelocity -= PN * A->InverseMass; A->AngularVelocity -= A->InverseInertiaWorld * glm::cross(rA, PN); }
                if (B->CanMove()) { B->LinearVelocity += PN * B->InverseMass; B->AngularVelocity += B->InverseInertiaWorld * glm::cross(rB, PN); }

                float maxF = mu * c.NormalImpulse;

                // ── Tangent 0 ──────────────────────────────────────────────
                vRel = B->VelocityAt(c.WorldPointB) - A->VelocityAt(c.WorldPointA);
                {
                    float emT = EffectiveMass(A, B, rA, rB, c.Tangent0);
                    if (emT > 1e-10f) {
                        float jT = -glm::dot(vRel, c.Tangent0) / emT;
                        float p0 = c.TangentImpulse0;
                        c.TangentImpulse0 = std::clamp(p0 + jT, -maxF, maxF);
                        glm::vec3 PT = c.Tangent0 * (c.TangentImpulse0 - p0);
                        if (A->CanMove()) { A->LinearVelocity -= PT * A->InverseMass; A->AngularVelocity -= A->InverseInertiaWorld * glm::cross(rA, PT); }
                        if (B->CanMove()) { B->LinearVelocity += PT * B->InverseMass; B->AngularVelocity += B->InverseInertiaWorld * glm::cross(rB, PT); }
                    }
                }

                // ── Tangent 1 ──────────────────────────────────────────────
                vRel = B->VelocityAt(c.WorldPointB) - A->VelocityAt(c.WorldPointA);
                {
                    float emT = EffectiveMass(A, B, rA, rB, c.Tangent1);
                    if (emT > 1e-10f) {
                        float jT = -glm::dot(vRel, c.Tangent1) / emT;
                        float p1 = c.TangentImpulse1;
                        c.TangentImpulse1 = std::clamp(p1 + jT, -maxF, maxF);
                        glm::vec3 PT = c.Tangent1 * (c.TangentImpulse1 - p1);
                        if (A->CanMove()) { A->LinearVelocity -= PT * A->InverseMass; A->AngularVelocity -= A->InverseInertiaWorld * glm::cross(rA, PT); }
                        if (B->CanMove()) { B->LinearVelocity += PT * B->InverseMass; B->AngularVelocity += B->InverseInertiaWorld * glm::cross(rB, PT); }
                    }
                }
            }
        }
    }

    // Split-impulse position correction.
    // Uses LOCAL contact coords to recompute lever arms after IntegratePositions
    // has moved the bodies, eliminating stale-WorldPoint lever-arm error.
    // MAX_COR raised from 0.02 → 0.2 m; with PositionIterations=3 passes
    // up to 60 cm of separation can be resolved per sub-step, preventing tunneling.
    void SolveContactPositions() {
        const float ERP = 0.3f;
        const float SLOP = 0.005f;
        const float MAX_COR = 0.2f;    // was 0.02 — caused tunneling on first contact

        for (auto& man : Contacts) {
            RigidBody* A = man.BodyA, * B = man.BodyB;
            for (auto& c : man.Contacts) {
                float pen = c.Depth - SLOP;
                if (pen <= 0.0f) continue;

                // Recompute lever arms from LOCAL coords (valid after position integration)
                glm::vec3 wA = A->LocalToWorld(c.LocalPointA);
                glm::vec3 wB = B->LocalToWorld(c.LocalPointB);
                glm::vec3 rA = wA - A->Position;
                glm::vec3 rB = wB - B->Position;

                // Also recompute actual penetration from updated positions
                // (stored c.Depth may be stale after previous correction passes)
                float actualPen = glm::dot(wB - wA, -man.Normal);
                pen = actualPen - SLOP;
                if (pen <= 0.0f) continue;

                float em = EffectiveMass(A, B, rA, rB, man.Normal);
                if (em < 1e-10f) continue;

                float corr = std::min(ERP * pen, MAX_COR) / em;
                glm::vec3 cv = man.Normal * corr;
                if (A->CanMove()) A->Position -= cv * A->InverseMass;
                if (B->CanMove()) B->Position += cv * B->InverseMass;
            }
        }
    }

    void TickSleep(float dt) {
        const float ls = SleepLinVelThreshold * SleepLinVelThreshold;
        const float as_ = SleepAngVelThreshold * SleepAngVelThreshold;
        for (auto* b : Bodies) {
            if (!b->IsDynamic()) continue;
            if (glm::length2(b->LinearVelocity) < ls && glm::length2(b->AngularVelocity) < as_) {
                b->SleepTimer += dt;
                if (b->SleepTimer >= SleepTimeThreshold) {
                    b->IsAwake = false; b->LinearVelocity = {}; b->AngularVelocity = {};
                }
            }
            else { b->SleepTimer = 0.0f; b->IsAwake = true; }
        }
    }
};