// Attempting to implement clouds as described in "Real-Time Cloud Rendering" 
// paper by Mark Harris and Anselmo Lastra, 2001.
// I may or may have not implemented it correctly.... though clouds plausible imho. 
// It is also rather slow - I am definitely not using noise properly. 

#version 450
#define PI 3.141592653589793 

const float MAXT = 150.0f;
const float FOGHEIGHT = 0.40; // background fog height. 
const float FOGFADEHEIGHT = 0.60; // background fog fade height - fades background fog into actual sky.
const vec3  FOGCOLOR = vec3(0.839, 1, 0.980);
const float FOGDENSITY = 0.009; 
const float FOGPOWER = 3.0;

const vec3  SUNDIRECTION = normalize(vec3(-1.0, -0.6, -0.9)); //direction of the sunlight
const vec3  SUNCOLOR =vec3(0.885, 0.980, 0.980); // sun color? 
const vec3  SKYCOLOR = vec3(0.513, 0.882, 0.945);

// colors for scene objects...
const vec3 TILE1_COLOR  = vec3(0.6);
const vec3 TILE2_COLOR  = vec3(0.8);

const vec3 CLOUD_ALBEDO = vec3(1.0);
const float CLOUDTRACE_NUMSTEPS = 20.0; // number of cloud samples. 
const float CLOUDTRACE_STEPSIZE = 0.1;  // spacing of cloud samples.


in vec2 fragTexCoord;
out vec4 fragColor;
uniform vec3 resolution;

uniform vec3 viewPosition;
uniform mat3 viewMatrix;
uniform float iGlobalTime;
uniform sampler2D iChannel0;

struct TraceResult {
    bool  hit;
    float rayt;
    float materialID;
};

// transformation funcs
mat3 rotateY(float n) {
    float a = cos(n);
    float b = sin(n);
    return mat3( a, 0.0, b, 0.0, 1.0, 0.0, -b, 0.0, a );
}

mat3 rotateX(float n) {
    float a = cos(n);
    float b = sin(n);
    return mat3( 1.0, 0.0, 0.0, 0.0, a, -b, 0.0, b, a );
}

mat3 rotateZ(float n) {
    float a = cos(n);
    float b = sin(n);
    return mat3(a, -b, 0.0, b, a, 0.0, 0.0, 0.0, 1.0);
}

// NOISE FUNCTIONS
float random(in vec3 st) { 
    return fract(sin(dot(st,vec3(12.9898,78.233,19.124)))*43758.5453);
}

float noise(in vec3 st) {
    vec3 i = floor(st);
    vec3 x = fract(st);
    vec3 u = x*x*(3.0-2.0*x);

    float a = random(i);
    float b = random(i + vec3(1.0, 0.0, 0.0));
    float c = random(i + vec3(0.0, 1.0, 0.0));
    float d = random(i + vec3(1.0, 1.0, 0.0));
    float e = random(i + vec3(0.0, 0.0, 1.0));
    float f = random(i + vec3(1.0, 0.0, 1.0));
    float g = random(i + vec3(0.0, 1.0, 1.0));
    float h = random(i + vec3(1.0, 1.0, 1.0));
    float fa = mix(a, b, u.x);
    float fb = mix(c, d, u.x);
    float fc = mix(e, f, u.x);
    float fd = mix(g, h, u.x);
    float fe = mix(fa, fb, u.y);
    float ff = mix(fc, fd, u.y);
    float fg = mix(fe, ff, u.z);
    return clamp(2.0*fg-1.0, -1.0, 1.0);
}

float fbm(vec3 p, int oct) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.5;

    for (int i = 0; i < oct; i++) {
        value += amplitude * noise(p*frequency);
        p *= 2.;
        amplitude *= 0.45;
        frequency *= 1.2;
    }
    return value;
}

// Some primitives and CSG ops.
float sdBox(vec3 p, vec3 b) {
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,vec3(0.0)));
}
float sdPlane(vec3 p, vec4 n) { return dot(p, n.xyz) - n.w; }
float opAdd(float d1, float d2) { return min(d1, d2); }
float opSubtract( float d1, float d2 ) { return max(-d1,d2); }

float clouds(vec3 p) {
    float f;
    f = (sdBox(p, vec3(9000.0, 1.35, 9000.0)));
    f -= noise(p * 0.4)*0.5+0.5;
    f = opSubtract(noise((p+vec3(iGlobalTime*0.05, 0.0, 0.0))*0.12+iGlobalTime*0.005)*0.8, f);
    f -= fbm(p*0.7+iGlobalTime*0.05, 2) *0.15;
    return f;
}

float map(vec3 p, out float mattype) {
    mattype = 1.0;
    float cl = clouds(p);
    float pl = sdPlane(p, vec4(0.0, 1.0, 0.0, -20.0));
    float f = opAdd(cl,pl);
    if (f == pl) mattype = 2.0;
    return f;
}

float map(vec3 p) { float unused; return map(p, unused); }

struct TraceResult trace(vec3 ro, vec3 rd) {
    struct TraceResult traceResult = TraceResult(false, 0.0, 0.0);
    float t = 0.02;
    float tmax = MAXT; 
    float mattype;

    for (; t < MAXT; ) {
        vec3 rp = ro + rd * t;
        float tr = map(rp, mattype);
        if (tr.x<0.0001) {
            traceResult = TraceResult(true, t, mattype);
            break;
        }
        t += tr.x;
    }
    traceResult.rayt = t;
    return traceResult;
}

//@ro - cloud starting position.
//@rd - view direction
//@ld - light direction 
vec3 traceCloud(vec3 ro, vec3 rd, vec3 ld) {
    float a = 1.118;  // transparency constant - each particle in the cloud will have this transparency.
    float b = 0.056;  // distance field- dependent transparency.
    float c = 0.896;  // air transparency;

    vec3 albedo = CLOUD_ALBEDO; 
    // phase functions for forward scattering and eye scattering, respectively
    float ph1 = 3.0/4.0*(cos(PI)*cos(PI)+1.0);
    float ph2 = 3.0/4.0*(1.0+dot(ld,rd)*dot(ld,rd));


    vec3 Ik = SUNCOLOR; 
    vec3 Ek = vec3(0.0); // eye scattering, initially no light is scattered...


    const float numsteps = CLOUDTRACE_NUMSTEPS;
    const float stepsize = CLOUDTRACE_STEPSIZE;

    for (float i = stepsize; i <= stepsize*numsteps; i+=stepsize) {
        // forward scattering
        vec3 rp = ro+(i-stepsize)*ld;
        float m = clouds(rp);
        float transparency = (m<0.1) ? 1.0/(a+abs(m)*b) : c;
        vec3 gk = albedo*transparency*Ik*ph1/(4.0*PI);
        Ik = gk+Ik*transparency;

        // compute eye scattering 
        rp = ro+stepsize*ld; 
        m = clouds(rp);
        transparency = (m<0.01) ? 1.0/(a+abs(m)*b) : c;
        vec3 sk = albedo*transparency*Ik*ph2/(4.0*PI);
        Ek = sk+transparency*Ek;
    }

    // not sure if averaging is correct...! 
    return clamp((Ik+Ek)*0.5, 0.0, 1.0); 
}

vec3 calcNormal(vec3 p) {
    vec2 eps = vec2(0.001,0.0);
    float x = map(p+eps.xyy)-map(p-eps.xyy);
    float y = map(p+eps.yxy)-map(p-eps.yxy);
    float z = map(p+eps.yyx)-map(p-eps.yyx);
    return normalize(vec3(x,y,z));
}

// softshadow function by iq. 
float traceShadow(vec3 ro, vec3 rd) {
    float numsteps = 30.0;
    float stepsize = 1.6;
    float res = 1.0;
    const float k = 200.0;

    for (float i = 0.0; i < numsteps *stepsize; i+=stepsize) {
        float d = map(ro+i*rd);
        if (d < 0.001) { return 0.0; }
        res = min( res, k*d/i );
    }
    return res;
}

float phongDiffuseFactor(vec3 l, vec3 n) {
    return max(0.0, dot(l,n));
}

float phongSpecularFactor(vec3 l, vec3 n, vec3 v, float k) {
    vec3 r = normalize(reflect(l, n));
    return pow(max(0.0, dot(r, v)), k);
}

float fog(float dist, float d, float p) {
    return  1.0 - 1.0/exp(pow(dist*d, p));
}

vec3 sky(vec3 ro, vec3 rd) {
    vec3 color = SKYCOLOR; 
    float d = dot(-SUNDIRECTION, rd); // sun??
    if (d > 0.0)          
        color = mix(color, vec3(1.0), pow(d, 150.0));
    if (rd.y < FOGFADEHEIGHT)     
        color = mix(FOGCOLOR, color, (rd.y-FOGHEIGHT)/(FOGFADEHEIGHT-FOGHEIGHT));
    if (rd.y < FOGHEIGHT) 
        color = FOGCOLOR;
    return clamp(color, 0.0, 1.0);
}

void main(void) {
    vec2 st = fragTexCoord;
    float finv = tan(90.0 * 0.5 * PI / 180.0);
    float aspect = resolution.x / resolution.y;
    st.x = st.x * aspect;
    st = (st - vec2(aspect * 0.5, 0.5)) * finv;


    vec3 rd = normalize(vec3(st, 1.0));
    //rd=rotateY(-1.55)*rd;
    rd = viewMatrix  * rd;
    rd = normalize(rd);

    //vec3 ro = vec3(0.25, 0.0, 0.0); 
    vec3 ro = viewPosition;
    //vec3 ro = vec3(3.0, 0.0, -7.0); 
    //ro += up * 0.22 * normalize(vec3(0.0, 1.0, 0.0));
    //ro += forward * 0.5*  normalize(vec3(0.0, 0.0, 1.0));
    //ro += iGlobalTime*0.50*normalize(vec3(0.0, 0.0, 1.0));


    vec3 color = sky(ro, rd);

    TraceResult traceResult = trace(ro, rd);
    if (traceResult.hit) {
        vec3 rp = ro+traceResult.rayt*rd;
        vec3 n = calcNormal(rp);

        if (traceResult.materialID == 1.0) {
            // get to cloud's start tracing position.
            vec3 crp = rp + (-SUNDIRECTION)*CLOUDTRACE_NUMSTEPS*CLOUDTRACE_STEPSIZE*1.001;
            color = 0.1+traceCloud(crp, rd, SUNDIRECTION);
            //fogpowr = 30.0; fogdens = 0.0067;
        }

        if (traceResult.materialID == 2.0) {
            vec3 tiles = (sin(rp.z) + sin(rp.x) <= 0.0) ? TILE1_COLOR : TILE2_COLOR;
            vec3 sp = rp + 0.1*n;
            float ph = phongDiffuseFactor(-SUNDIRECTION, n) 
                     + phongSpecularFactor(-SUNDIRECTION, n, rd, 10.0);
            color = 0.5*SUNCOLOR + ph * traceShadow(sp, -SUNDIRECTION);
            color = tiles * color;
        }

        float fg = fog(traceResult.rayt, FOGDENSITY, FOGPOWER);
        vec3 fc = FOGCOLOR;
        color = mix(color, fc, fg);
    }

    color = clamp(color, 0.0, 1.0);
    fragColor = vec4(color, 1.0); 
}