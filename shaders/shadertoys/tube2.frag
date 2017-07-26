// Trying some some more tentacles.... this time with cell noise. 
#version 450
#define PI 3.141592653589793 

const float MAXT = 70.0f;
const float FOGHEIGHT = 0.01; // background fog height. 
const float FOGFADEHEIGHT = 0.30; // background fog fade height - fades background fog into actual sky.
const vec3  FOGCOLOR = vec3(0.839, 1, 0.980);
const float FOGDENSITY = 0.022; 


in vec2 fragTexCoord;
out vec4 fragColor;
uniform vec2 resolution;
uniform vec2 mouse;
uniform float forward;
uniform float up;
uniform float iGlobalTime;

struct TraceResult {
    bool hit;
    float rayt;
    vec3 color; 
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
float random(in float n) {
    return fract(sin(n)*43758.5453);
}

float random(in vec2 st) { 
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float random(in vec3 st) { 
    return fract(sin(dot(st,vec3(12.9898,78.233,19.124)))*43758.5453);
}

float noise(in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    
    vec2 u = f*f*(3.0-2.0*f);
    float a1 = mix(a, b, u.x);
    float a2 = mix(c, d, u.x);
    float a3 = mix(a1, a2, u.y);
    return clamp(a3, 0.0, 1.0); 
}

float noise(in vec3 st) {
    vec3 i = floor(st);
    vec3 x = fract(st);

    float a = random(i);
    float b = random(i + vec3(1.0, 0.0, 0.0));
    float c = random(i + vec3(0.0, 1.0, 0.0));
    float d = random(i + vec3(1.0, 1.0, 0.0));
    float e = random(i + vec3(0.0, 0.0, 1.0));
    float f = random(i + vec3(1.0, 0.0, 1.0));
    float g = random(i + vec3(0.0, 1.0, 1.0));
    float h = random(i + vec3(1.0, 1.0, 1.0));
    vec3 u = x*x*(3.0-2.0*x);
    float fa = mix(a, b, u.x);
    float fb = mix(c, d, u.x);
    float fc = mix(e, f, u.x);
    float fd = mix(g, h, u.x);
    float fe = mix(fa, fb, u.y);
    float ff = mix(fc, fd, u.y);
    float fg = mix(fe, ff, u.z);
    return clamp(2.0*fg-1.0, -1.0, 1.0);
}

float sdSpheroid(vec3 p, vec3 s) {
    return (length(p/s) - 1.0)  * min(s.x, min(s.y, s.z));
}

float sdPlane(vec3 p, vec4 n) {
    return dot(p, n.xyz) - n.w;
}

// polynomial smooth min (k = 0.1);
float opAddSmooth( float a, float b, float k ) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float opSubtract( float a, float b ) {
    return max(-a,b);
}

float opAdd(float a, float b) {
    return min(a,b);
}

vec3 opRepeat(vec3 p, vec3 c) {
    return mod(p,c)-0.5*c;
}

float worley_smooth(vec3 p) {
    vec3 tileCoord = floor(p);    
    float dist = 90000.0;
    float heightoffset = 0.0;
    for (int z = -1; z <= 1; z++)
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        vec3 currentTile = tileCoord + vec3(x,y,z);
        vec3 point = currentTile + random(currentTile);

        float d = distance(point, p);
        dist = opAddSmooth(dist, d, 0.05);
    }
    return -dist;
}

float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 0.8;

    const float octaves = 5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p*frequency);
        p *= 2.;
        amplitude *= 0.3;
        frequency *= 1.4;
    }
    return value;
}

vec4 map(vec3 p) {
    // tentacle
    p.x -= noise(p.zz*0.2+iGlobalTime*0.02) * 2.25;
    p.y -= noise(p.xy*0.2+iGlobalTime*0.08) * 1.20;
    float tube = length(p.xy)-1.75; 

    for (int i = 1; i < 6; i++) {
        vec3 pt = p - vec3(0.0,0.0,float(i)*20.0);
        pt = rotateZ(1.75*float(i))*pt;
        pt = opRepeat(pt, vec3(0.0, 0.0, 60.0));

        float sphere = sdSpheroid(pt-vec3(0.0, 12.0, 0.0), vec3(6.0, 5.0, 6.0));
        float stube = length(pt.xz) - 1.2;
        float plane = sdPlane(pt, vec4(0.0,1.0,0.0,0.0));

        stube = opSubtract(plane, stube);
        stube = opAddSmooth(sphere, stube, 5.5);
        tube = opAddSmooth(stube, tube, 0.75);
    }

    // outer walls
    float box1 = length(p.xy) -12.0;
    float box2 = length(p.xy) -10.0;
    box1 = opSubtract(box2, box1);
    tube = opAddSmooth(box1, tube, 4.0);

    vec3 color = vec3(0.964, 0.482, 0.454); // default body color;
    vec3 cavity_color = vec3(0.403, 0.090, 0.074); // color between cells.
    vec3 veins_color = vec3(0.027, 0.117, 0.333);

    float f = worley_smooth(p*0.5-iGlobalTime*0.02)*0.7;
    color = mix(color, cavity_color, abs(f)*2.5);  

    // veins @OPTIMIZE this is still slow!!! 
    float a = 0.25*(1.0-abs(fbm(p*1.0+iGlobalTime*0.04))); 
    float c = 0.01*(1.0-abs(fbm(p*9.2+iGlobalTime*0.01))); 
    float veins = a+c;
    float sm = smoothstep(0.0, 1.0, veins*2.0);
    color = mix(color, veins_color, sm);

    return vec4((tube-f-veins)*0.75, color);
}

struct TraceResult trace(vec3 ro, vec3 rd) {
    struct TraceResult traceResult = TraceResult(false, 0.0, vec3(0.0));
    float t = 0.02;
    float tmax = MAXT; 
    for (;t < tmax;) {
        vec3 rp = ro + rd * t;
        vec4 tr = map(rp);
        if (tr.x<0.001) {
            traceResult = TraceResult(true, t, tr.yzw);
            break;
        }
        t += tr.x;
    }
    traceResult.rayt = t;
    return traceResult;
}


vec3 calcNormal(vec3 p) {
    vec2 eps = vec2(0.001,0.0);
    float x = map(p+eps.xyy).x-map(p-eps.xyy).x;
    float y = map(p+eps.yxy).x-map(p-eps.yxy).x;
    float z = map(p+eps.yyx).x-map(p-eps.yyx).x;
    return normalize(vec3(x,y,z));
}

// based on a function in IQ's shadertoy 
//https://www.shadertoy.com/view/Xdl3R4
float calcSurfaceThickness(vec3 ro, vec3 rd, float p) {
    float w = 1.0;
    float a = 0.0;
    const int numsteps = 4;
    for (int t = 1; t <= numsteps; t++) {
        float rt = 0.1*float(t); 
        float d = map(ro+rt*rd).x;
        a += w*(rt-min(d,0.0));
        w *= 0.9;
    }

    return pow(clamp(1.2-0.25*a, 0.0, 1.0), p);
}

float calcAO(vec3 p, vec3 n) {
    float w = 1.0;
    float a = 0.0;
    const int maxsteps = 4;
    for (int t = 1; t <= maxsteps; t++) { 
        float rt = 0.2*float(t);
    	a += w*(rt-map(p+rt*n).x);   
        w *= 0.8;
    }
    
    return clamp(1.0-a, 0.0, 1.0);
}

//@l - light direction, normalized;
//@n - surface normal, normalized;
float phongDiffuseFactor(vec3 l, vec3 n) {
    return max(0.0, dot(l,n));
}

//@l - light direction, normalized;
//@n - surface normal, normalized;
//@v - view direction, normalized;
//@k - shininess constant;
float phongSpecularFactor(vec3 l, vec3 n, vec3 v, float k) {
    vec3 r = normalize(reflect(l, n));
    return pow(max(0.0, dot(r, v)), k);
}

float fog(float dist) {
    return  1.0 - 1.0/exp(pow(dist*FOGDENSITY, 2.0));
}


void main(void) {
    vec2 st = fragTexCoord;
    float finv = tan(90.0 * 0.5 * PI / 180.0);
    float aspect = resolution.x / resolution.y;
    st.x = st.x * aspect;
    st = (st - vec2(aspect * 0.5, 0.5)) * finv;

    vec3 rd = normalize(vec3(st, 1.0));
    rd = rotateY(-mouse.x) * rotateX(mouse.y)  * rd;
    rd = normalize(rd);

    vec3 ro = vec3(2.0, 4.0, 0.0); 
    ro += up * 0.22 * normalize(vec3(0.0, 1.0, 0.0));
    ro += forward * 0.5*  normalize(vec3(0.0, 0.0, 1.0));
    ro += iGlobalTime*0.3*normalize(vec3(0.0, 0.0, 1.0));

    vec3 color = FOGCOLOR;
    TraceResult traceResult = trace(ro, rd);
    if (traceResult.hit) {

        vec3 rp = ro+traceResult.rayt*rd;

        vec3 n = calcNormal(rp);

        // calculate surface thickness 
        float thickness = calcSurfaceThickness(rp-n*0.01, rd, 20.0) * 4.0;
        float ao = calcAO(rp, n);

        vec3 c = traceResult.color;

        vec3 SUNDIRECTION = rd;
        float ph = 0.5*ao + phongDiffuseFactor(-SUNDIRECTION, n) * 0.5 
                          + phongSpecularFactor(-SUNDIRECTION,n, rd, 50.0) * 0.5;
        float ph2 = phongDiffuseFactor(-SUNDIRECTION, n) ;
        color = ph*c+c*thickness;
        color = mix(color, FOGCOLOR, fog(traceResult.rayt));
    }

    color = clamp(color, 0.0, 1.0);
    fragColor = vec4(color, 1.0); 
}