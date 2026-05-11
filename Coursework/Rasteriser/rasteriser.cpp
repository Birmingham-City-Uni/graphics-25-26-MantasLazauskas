#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <limits>
#include <array>
#include <iostream>
#include <lodepng.h>

struct Vec2
{
    float x = 0.0f, y = 0.0f; Vec2() {} Vec2(float X, float Y) :x(X), y(Y) {}
};
struct Vec3
{
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() {} Vec3(float X, float Y, float Z) :x(X), y(Y), z(Z) {}
    Vec3 operator+(const Vec3& o) const
    {
        return Vec3(x + o.x, y + o.y, z + o.z);
    }

    Vec3 operator-(const Vec3& o) const
    {
        return Vec3(x - o.x, y - o.y, z - o.z);
    }

    Vec3 operator*(float s) const
    {
        return Vec3(x * s, y * s, z * s);
    }

    Vec3 operator/(float s) const
    {
        return Vec3(x / s, y / s, z / s);
    }

    Vec3 operator*(const Vec3& o) const
    {
        return Vec3(x * o.x, y * o.y, z * o.z);
    }

    Vec3& operator+=(const Vec3& o)
    {
        x += o.x; y += o.y; z += o.z; return *this;
    }
};
inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float length(const Vec3& v)
{
    return std::sqrt(dot(v, v));
}

inline Vec3 normalize(const Vec3& v)
{
    float L = length(v); return L > 0.0f ? v / L : v;
}

struct Vec4
{
    float x, y, z, w; Vec4() {} Vec4(float a, float b, float c, float d) :x(a), y(b), z(c), w(d) {}
};

struct Mat4
{
    float m[4][4];
    static Mat4 identity()
    {
        Mat4 R = {};
        for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) R.m[i][j] = (i == j) ? 1.0f : 0.0f;
        return R;
    }
    static Mat4 translate(const Vec3& t)
    {
        Mat4 M = identity();
        M.m[0][3] = t.x; M.m[1][3] = t.y; M.m[2][3] = t.z;
        return M;
    }
    static Mat4 scale(const Vec3& s)
    {
        Mat4 M = identity();
        M.m[0][0] = s.x; M.m[1][1] = s.y; M.m[2][2] = s.z;
        return M;
    }

    static Mat4 rotateY(float r)
    {
        Mat4 M = identity();
        float c = std::cos(r), s = std::sin(r);
        M.m[0][0] = c;  M.m[0][2] = s;
        M.m[2][0] = -s; M.m[2][2] = c;
        return M;
    }

    Vec4 mul(const Vec4& v) const
    {
        Vec4 r;
        r.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
        r.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
        r.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
        r.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
        return r;
    }

    Mat4 operator*(const Mat4& o) const
    {
        Mat4 R = {};
        for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++)
        {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += m[i][k] * o.m[k][j];
            R.m[i][j] = s;
        }
        return R;
    }
};

Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
    Vec3 f = normalize(center - eye);
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);
    Mat4 M = Mat4::identity();
    M.m[0][0] = s.x; M.m[0][1] = s.y; M.m[0][2] = s.z;
    M.m[1][0] = u.x; M.m[1][1] = u.y; M.m[1][2] = u.z;
    M.m[2][0] = -f.x; M.m[2][1] = -f.y; M.m[2][2] = -f.z;
    M.m[0][3] = -dot(s, eye);
    M.m[1][3] = -dot(u, eye);
    M.m[2][3] = dot(f, eye);
    return M;
}

Mat4 perspective(float fovDeg, float aspect, float zn, float zf)
{
    float fov = fovDeg * (3.14159265f / 180.0f);
    float f = 1.0f / std::tan(0.5f * fov);
    Mat4 P = {};
    P.m[0][0] = f / aspect;
    P.m[1][1] = f;
    P.m[2][2] = (zf + zn) / (zn - zf);
    P.m[2][3] = (2.0f * zf * zn) / (zn - zf);
    P.m[3][2] = -1.0f;
    return P;
}

// Texture: stores RGBA image loaded by lodepng.
// Output/input PNG operations use lodepng.
struct Texture
{
    unsigned width = 0, height = 0;
    std::vector<unsigned char> data; // RGBA
    bool load(const std::string& path)
    {
        unsigned err = lodepng::decode(data, width, height, path);
        if (err) {
            data.clear(); return false;
        }
        return true;
    }
    bool valid() const
    {
        return !data.empty();
    }
};

struct Material
{
    // Material properties used in simple Phong-like shading
    std::string name;
    Vec3 Kd = Vec3(0.8f, 0.8f, 0.8f); // diffuse tint
    Vec3 Ks = Vec3(0.0f, 0.0f, 0.0f); // specular color
    float Ns = 10.0f;                // shininess (spec power)
    std::string map_kd;
    Texture tex;                      // texture
};

struct Mesh
{
    struct Vertex
    {
        Vec3 p; Vec3 n; Vec2 uv;
    };
    std::vector<Vertex> verts; // per-vertex attributes (position, normal, uv)
    std::vector<unsigned> idx; // indices (triangles)
    std::string matName;       // material name for this mesh
};

static std::string dirname(const std::string& p)
{
    size_t pos = p.find_last_of("\\/");
    return (pos == std::string::npos) ? std::string() : p.substr(0, pos + 1);
}

// Minimal MTL loader:
// Reads Kd, Ks, Ns and map_Kd
// Loads texture files referenced by map_Kd via Texture::load
bool loadMtl(const std::string& path, std::unordered_map<std::string, Material>& out, const std::string& base = "")
{
    std::ifstream in(path);
    if (!in) return false;
    Material cur; std::string line;
    auto flush = [&]()
        {
            if (!cur.name.empty()) out[cur.name] = cur; cur = Material();
        };
    while (std::getline(in, line))
    {
        std::istringstream ss(line); std::string tok; ss >> tok;
        if (tok == "newmtl")
        {
            flush(); ss >> cur.name;
        }
        else if (tok == "Kd") ss >> cur.Kd.x >> cur.Kd.y >> cur.Kd.z;
        else if (tok == "Ks") ss >> cur.Ks.x >> cur.Ks.y >> cur.Ks.z;
        else if (tok == "Ns") ss >> cur.Ns;
        else if (tok == "map_Kd")
        {
            ss >> cur.map_kd;
            if (!base.empty()) cur.map_kd = base + cur.map_kd;
        }
    }
    flush();
    for (auto& kv : out) if (!kv.second.map_kd.empty()) kv.second.tex.load(kv.second.map_kd);
    return true;
}

  // OBJ loader:
  // Reads positions (v), texcoords (vt), normals (vn), faces (f)
  // Builds per-material mesh groups
bool loadObj(const std::string& path, std::vector<Mesh>& meshes, std::unordered_map<std::string, Material>& mats) {
    std::ifstream in(path);
    if (!in) return false;
    std::string base = dirname(path);
    std::vector<Vec3> positions; std::vector<Vec2> texs; std::vector<Vec3> norms;
    std::string curMat; std::unordered_map<std::string, Mesh> groups; std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line); std::string tok; ss >> tok;
        if (tok == "v") { Vec3 p; ss >> p.x >> p.y >> p.z; positions.push_back(p); }
        else if (tok == "vt") { Vec2 t; ss >> t.x >> t.y; texs.push_back(t); }
        else if (tok == "vn") { Vec3 n; ss >> n.x >> n.y >> n.z; norms.push_back(n); }
        else if (tok == "mtllib") { std::string m; ss >> m; loadMtl(base + m, mats, base); }
        else if (tok == "usemtl") { ss >> curMat; }
        else if (tok == "f") {
            std::vector<std::tuple<int, int, int>> face; std::string v;
            while (ss >> v) {
                int vi = 0, ti = 0, ni = 0;
                size_t p1 = v.find('/');
                if (p1 == std::string::npos) vi = std::stoi(v);
                else {
                    size_t p2 = v.find('/', p1 + 1);
                    vi = std::stoi(v.substr(0, p1));
                    if (p2 == std::string::npos) ti = std::stoi(v.substr(p1 + 1));
                    else {
                        if (p2 > p1 + 1) ti = std::stoi(v.substr(p1 + 1, p2 - p1 - 1));
                        if (p2 + 1 < v.size()) ni = std::stoi(v.substr(p2 + 1));
                    }
                }
                face.emplace_back(vi, ti, ni);
            }
            if (face.size() < 3) continue;
            Mesh& mesh = groups[curMat]; mesh.matName = curMat;
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                std::array<std::tuple<int, int, int>, 3> tri = { { face[0], face[i], face[i + 1] } };
                for (int k = 0; k < 3; k++) {
                    int vi = std::get<0>(tri[k]), ti = std::get<1>(tri[k]), ni = std::get<2>(tri[k]);
                    Mesh::Vertex vertv;
                    vertv.p = positions[vi > 0 ? (vi - 1) : (positions.size() + vi)];
                    vertv.uv = (ti != 0) ? texs[ti > 0 ? (ti - 1) : (texs.size() + ti)] : Vec2(0, 0);
                    vertv.n = (ni != 0) ? norms[ni > 0 ? (ni - 1) : (norms.size() + ni)] : Vec3(0, 0, 1);
                    mesh.verts.push_back(vertv);
                    mesh.idx.push_back((unsigned)mesh.verts.size() - 1);
                }
            }
        }
    }
    for (auto& kv : groups) meshes.push_back(kv.second);
    for (auto& kv : mats) { if (!kv.second.map_kd.empty() && !kv.second.tex.valid()) kv.second.tex.load(kv.second.map_kd); }
    return true;
}

inline float edge(const Vec3& a, const Vec3& b, const Vec3& c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

enum LightType
{
    DIRECTIONAL, POINT
};

struct Light
{
    LightType type = DIRECTIONAL;
    Vec3 dir; Vec3 pos;
    Vec3 color = Vec3(1, 1, 1);
    float intensity = 1.0f;
};

  // Camera: perspective camera used by the rasteriser.
  // Move camera in code by changing Camera::eye / target / fov.

struct Camera
{
    Vec3 eye{ 0,1.2f,20.0f }; // change this to move camera back/forward
    Vec3 target{ 0,0.8f,0 };
    Vec3 up{ 0,1,0 };
    float fov = 45.0f;
    float zn = 0.1f, zf = 200.0f;
};

int main()
{
    const int outW = 1920, outH = 1080;
    const int AA = 2; // anti-aliasing factor: render at higher res then downsample
    const int W = outW * AA, H = outH * AA;
    const int channels = 4;
    const std::string outFile = "output.png";

    // background + models
    const std::string modelDir = "../../../Coursework/Rasteriser/models/";
    std::vector<unsigned char> bg;
    unsigned bgW = 0, bgH = 0;
    bool haveBg = false;
    // Output
    if (lodepng::decode(bg, bgW, bgH, modelDir + "FOC_Background.png") == 0) haveBg = true;
    else if (lodepng::decode(bg, bgW, bgH, "./models/FOC_Background.png") == 0) haveBg = true;
    if (!haveBg) std::cerr << "Warning: could not load background (tried modelDir and ./models)\n";

    std::vector<Mesh> meshes;
    std::unordered_map<std::string, Material> materials;

    // simple diagnostics for model files and load attempts (input check)
    auto fileExists = [](const std::string& p)->bool { std::ifstream f(p); return f.good(); };
    std::cout << "Model dir: " << modelDir << "\n";
    std::string names[] = { "optimus_prime.obj","bumblebee.obj","grimlock.obj","metroplex.obj" };
    for (const auto& n : names) {
        std::string p = modelDir + n;
        std::cout << "Exists? " << p << " -> " << (fileExists(p) ? "YES" : "NO") << "\n";
    }

    // load models (OBJ+MTL parsing) - returns false if file open fails
    if (!loadObj(modelDir + "optimus_prime.obj", meshes, materials)) std::cerr << "Failed to load optimus_prime.obj\n";
    if (!loadObj(modelDir + "bumblebee.obj", meshes, materials)) std::cerr << "Failed to load bumblebee.obj\n";
    if (!loadObj(modelDir + "grimlock.obj", meshes, materials)) std::cerr << "Failed to load grimlock.obj\n";
    if (!loadObj(modelDir + "metroplex.obj", meshes, materials)) std::cerr << "Failed to load metroplex.obj\n";

    std::cout << "Loaded meshes: " << meshes.size() << "\n";
    for (size_t i = 0; i < meshes.size(); ++i) {
        std::cout << " mesh[" << i << "] mat='" << meshes[i].matName << "' verts=" << meshes[i].verts.size() << " idx=" << meshes[i].idx.size() << "\n";
    }

    // camera & matrices (perspective)
    Camera cam;
    Mat4 view = lookAt(cam.eye, cam.target, cam.up);
    Mat4 proj = perspective(cam.fov, float(W) / float(H), cam.zn, cam.zf);

    // lights (directional + point), used in shading below
    std::vector<Light> lights;
    Light key; key.type = DIRECTIONAL; key.dir = normalize(Vec3(-0.4f, -0.7f, -0.5f)); key.color = Vec3(1.0f, 0.95f, 0.9f); key.intensity = 1.2f;
    Light fill; fill.type = POINT; fill.pos = Vec3(2.5f, 3.0f, 4.0f); fill.color = Vec3(0.9f, 0.9f, 1.0f); fill.intensity = 0.6f;
    lights.push_back(key); lights.push_back(fill);

    // Z-buffer (depth buffer) initialization
    std::vector<float> zbuf(W * H, std::numeric_limits<float>::infinity());
    std::vector<unsigned char> frame(W * H * channels, 0);

    // fill background (alpha blend with later rasterised pixels)
    if (haveBg) {
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
            int sx = x * (int)bgW / W;
            int sy = y * (int)bgH / H;
            size_t bi = (sy * bgW + sx) * 4;
            size_t fi = (y * W + x) * 4;
            frame[fi + 0] = bg[bi + 0]; frame[fi + 1] = bg[bi + 1]; frame[fi + 2] = bg[bi + 2]; frame[fi + 3] = 255;
        }
    }
    else {
        for (size_t i = 0; i < frame.size(); i += 4) { frame[i + 0] = 20; frame[i + 1] = 20; frame[i + 2] = 30; frame[i + 3] = 255; }
    }

    // transform pipeline: model -> view -> projection
    auto transform = [&](const Vec3& p, const Mat4& model)->Vec4 {
        Vec4 wp = model.mul(Vec4(p.x, p.y, p.z, 1.0f));
        Vec4 vv = view.mul(wp);
        return proj.mul(vv);
        };

    // model transforms: positions/scales for each mesh (edit to place/scale models)
    std::vector<Mat4> models;
    models.reserve(meshes.size());
    for (size_t i = 0; i < meshes.size(); ++i)
    {
        Mat4 M = Mat4::identity();
        if (i == 0) // optimus_prime
            M = Mat4::translate(Vec3(0.0f, -1.1f, 0.2f)) * Mat4::rotateY(0.0f) * Mat4::scale(Vec3(1.4f, 1.4f, 1.4f));
        else if (i == 1) // bumblebee
            M = Mat4::translate(Vec3(-2.2f, -1.25f, 0.6f)) * Mat4::rotateY(0.3f) * Mat4::scale(Vec3(0.95f, 0.95f, 0.95f));
        else if (i == 2) // grimlock
            M = Mat4::translate(Vec3(1.8f, -1.3f, 0.7f)) * Mat4::rotateY(-0.2f) * Mat4::scale(Vec3(1.1f, 1.1f, 1.1f));
        else if (i == 3) // metroplex
            M = Mat4::translate(Vec3(0.0f, -1.7f, -2.5f)) * Mat4::rotateY(0.5f) * Mat4::scale(Vec3(3.2f, 3.2f, 3.2f));
        else // fallback
            M = Mat4::translate(Vec3(0.0f, -1.0f, 0.6f)) * Mat4::scale(Vec3(1.0f, 1.0f, 1.0f));

        models.push_back(M);
    }

     // Shading:
     // Simple diffuse + specular (Ns) phong-like shading
     // Supports directional and point lights; point uses distance attenuation
     // Small ambient term

    Vec3 currentMeshColor = Vec3(1.0f, 1.0f, 1.0f);
    auto shade = [&](const Material& m, const Vec3& posW, const Vec3& Nw, const Vec2& uv)->Vec3 {
        Vec3 N = normalize(Nw);
        Vec3 baseColor = currentMeshColor * m.Kd; // material tint
        Vec3 color = baseColor * 0.05f; // ambient
        for (const Light& L : lights) {
            Vec3 Ldir; float att = 1.0f;
            if (L.type == DIRECTIONAL) Ldir = normalize(L.dir) * -1.0f;
            else { Ldir = normalize(L.pos - posW); float d = length(L.pos - posW); att = 1.0f / (1.0f + 0.09f * d + 0.032f * d * d); }
            float NdotL = std::max(0.0f, dot(N, Ldir));
            Vec3 V = normalize(cam.eye - posW);
            Vec3 H = normalize(Ldir + V);
            float specPow = std::pow(std::max(0.0f, dot(N, H)), m.Ns);
            Vec3 diffuse = baseColor * NdotL;
            Vec3 specular = m.Ks * specPow;
            color += (diffuse + specular) * L.color * L.intensity * att;
        }
        color.x = std::min(1.0f, color.x); color.y = std::min(1.0f, color.y); color.z = std::min(1.0f, color.z);
        return color;
        };

     // Rasterisation loop (triangle rasterization):
      // Project vertices to NDC then screen
      // Compute triangle bounding box and barycentric coordinates
      // Perspective-correct interpolation (using 1/w)
      // Depth test using zbuf (Z-buffer)
      // Writes shaded color to frame

    for (size_t mi = 0; mi < meshes.size(); ++mi) {
        const Mesh& mesh = meshes[mi];
        Mat4 model = models[mi];
        currentMeshColor = Vec3(0.8f, 0.8f, 0.8f); // neutral tint
        for (size_t t = 0; t + 2 < mesh.idx.size(); t += 3) {
            const auto& A = mesh.verts[mesh.idx[t + 0]];
            const auto& B = mesh.verts[mesh.idx[t + 1]];
            const auto& C = mesh.verts[mesh.idx[t + 2]];

            Vec4 Ac = transform(A.p, model);
            Vec4 Bc = transform(B.p, model);
            Vec4 Cc = transform(C.p, model);
            if (Ac.w == 0.0f || Bc.w == 0.0f || Cc.w == 0.0f) continue;

            Vec3 A_ndc(Ac.x / Ac.w, Ac.y / Ac.w, Ac.z / Ac.w);
            Vec3 B_ndc(Bc.x / Bc.w, Bc.y / Bc.w, Bc.z / Bc.w);
            Vec3 C_ndc(Cc.x / Cc.w, Cc.y / Cc.w, Cc.z / Cc.w);

            Vec3 sA{ (A_ndc.x + 1.0f) * 0.5f * W, (1.0f - A_ndc.y) * 0.5f * H, A_ndc.z };
            Vec3 sB{ (B_ndc.x + 1.0f) * 0.5f * W, (1.0f - B_ndc.y) * 0.5f * H, B_ndc.z };
            Vec3 sC{ (C_ndc.x + 1.0f) * 0.5f * W, (1.0f - C_ndc.y) * 0.5f * H, C_ndc.z };

            Vec3 e1 = sB - sA, e2 = sC - sA;
            float cz = e1.x * e2.y - e1.y * e2.x;
            // Back-face culling: skip triangles facing away
            // if(cz <= 0.0f) continue;

            int minX = std::max(0, (int)std::floor(std::min(std::min(sA.x, sB.x), sC.x)));
            int minY = std::max(0, (int)std::floor(std::min(std::min(sA.y, sB.y), sC.y)));
            int maxX = std::min(W - 1, (int)std::ceil(std::max(std::max(sA.x, sB.x), sC.x)));
            int maxY = std::min(H - 1, (int)std::ceil(std::max(std::max(sA.y, sB.y), sC.y)));

            float triArea = edge(sA, sB, sC);
            if (std::abs(triArea) < 1e-6f) continue;

            auto mulPos = [&](const Vec3& p)->Vec3 { Vec4 wp = model.mul(Vec4(p.x, p.y, p.z, 1.0f)); return Vec3(wp.x, wp.y, wp.z); };
            Vec3 Aw = mulPos(A.p), Bw = mulPos(B.p), Cw = mulPos(C.p);
            Vec3 An = normalize(A.n), Bn = normalize(B.n), Cn = normalize(C.n);

            Material mat;
            auto it = materials.find(mesh.matName);
            if (it != materials.end()) mat = it->second;

            for (int y = minY; y <= maxY; ++y) for (int x = minX; x <= maxX; ++x)
            {
                Vec3 P(float(x) + 0.5f, float(y) + 0.5f, 0.0f);
                float w0 = edge(sB, sC, P), w1 = edge(sC, sA, P), w2 = edge(sA, sB, P);
                if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                float a = w0 / triArea, b = w1 / triArea, c = w2 / triArea;

                // perspective-correct interpolation using 1/w
                float wa = 1.0f / Ac.w, wb = 1.0f / Bc.w, wc = 1.0f / Cc.w;
                float r = a * wa + b * wb + c * wc;
                if (r == 0.0f) continue;
                float invR = 1.0f / r;
                float ndcZ = (a * (A_ndc.z * wa) + b * (B_ndc.z * wb) + c * (C_ndc.z * wc)) * invR;
                float depth01 = ndcZ * 0.5f + 0.5f;
                int idx = y * W + x;
                if (depth01 < 0.0f || depth01 > 1.0f) continue;
                if (depth01 >= zbuf[idx]) continue; // depth test (Z-buffer)
                zbuf[idx] = depth01;

                // interpolate UVs (perspective-correct)
                Vec2 uv;
                float ua = wa * A.uv.x, ub = wb * B.uv.x, uc = wc * C.uv.x;
                float va = wa * A.uv.y, vb = wb * B.uv.y, vc = wc * C.uv.y;
                uv.x = (a * ua + b * ub + c * uc) * invR;
                uv.y = (a * va + b * vb + c * vc) * invR;

                Vec3 Pw = Aw * a + Bw * b + Cw * c;
                Vec3 Nw = normalize(An * a + Bn * b + Cn * c);

                // shading (diffuse + specular).
                Vec3 shaded = shade(mat, Pw, Nw, uv);
                size_t fi = (idx) * 4;
                frame[fi + 0] = (unsigned char)(shaded.x * 255.0f);
                frame[fi + 1] = (unsigned char)(shaded.y * 255.0f);
                frame[fi + 2] = (unsigned char)(shaded.z * 255.0f);
                frame[fi + 3] = 255;
            }
        }
    }

      //Anti-aliasing:
      // Rendered at W,H = outW * AA, outH * AA
      // Downsample by averaging AA*AA samples per output pixel

    std::vector<unsigned char> out(outW * outH * channels);
    for (int y = 0; y < outH; ++y) for (int x = 0; x < outW; ++x)
    {
        int sx = x * AA, sy = y * AA;
        int r = 0, g = 0, b = 0, a = 0;
        for (int yy = 0; yy < AA; ++yy) for (int xx = 0; xx < AA; ++xx)
        {
            int ix = sx + xx, iy = sy + yy;
            size_t idx = (iy * W + ix) * 4;
            r += frame[idx + 0]; g += frame[idx + 1]; b += frame[idx + 2]; a += frame[idx + 3];
        }
        int samples = AA * AA;
        size_t oi = (y * outW + x) * 4;
        out[oi + 0] = (unsigned char)(r / samples);
        out[oi + 1] = (unsigned char)(g / samples);
        out[oi + 2] = (unsigned char)(b / samples);
        out[oi + 3] = (unsigned char)(a / samples);
    }

    // Output: write PNG using lodepng (visualise the render)
    unsigned err = lodepng::encode(outFile, out, outW, outH);
    if (err) { std::cerr << "Encode failed: " << lodepng_error_text(err) << "\n"; return 1; }
    std::cout << "Saved " << outFile << "\n";
    return 0;
}