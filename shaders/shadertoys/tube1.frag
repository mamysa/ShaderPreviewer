// Trying to make some weird organic tentacle-like things. Doesn't look too organic though, need to
// mess around with noise a bit more.
#version 450
#define PI 3.141592653589793 

#define MATERIAL_EXTERIOR 1 
#define MATERIAL_INTERIOR 2 

const float MAXT = 20.0f;

const float FOGHEIGHT = 0.02; // background fog height. 
const float FOGFADEHEIGHT = 0.05; // background fog fade height - fades background fog into actual sky.
const vec3  FOGCOLOR = vec3(0.364, 0.270, 0.254);
const float FOGDENSITY = 0.08; 

const vec3  SUNDIRECTION = normalize(vec3(0.0, -0.3, -0.6)); //direction of the sunlight
const vec3  SUNCOLOR =vec3(1.0, 0.949, 0.839); // sun color? 
const vec3  SKYCOLOR = vec3(0.929, 0.792, 0.768);
const float REFRETA = 1.0/1.4;
const vec3 REPEATS = vec3(3.5, 0.0, 0.0);



const float EXTERIOR_FRESNEL_POW = 2.0;
const float ABSORBANCE_SCALE = 4.5;
const vec3  ABSORBANCE = vec3(0.15, 0.7, 0.8);
const float TRANSPARENCY = 1.9;

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
    int materialID;
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
    return clamp(fg, 0.0, 1.0);
}


// polynomial smooth min (k = 0.1); by iq.
#if 0
float opAddSmooth(float a, float b, float k) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix(b,a,h)-k*h*(1.0-h);
}
#endif

float opSubtract(float a, float b) {
    return max(-a,b);
}

float opAdd(float a, float b) {
    return min(a,b);
}

vec3 opRepeat(vec3 p, vec3 c) {
    return mod(p,c)-0.5*c;
}

float exterior(vec3 p, out int mattype) {
    mattype = MATERIAL_EXTERIOR;

    p.x -= noise(p.zz*0.1+iGlobalTime*0.02) * 3.0;
    p = opRepeat(p, REPEATS);
    float f;
    f = length(p.xy)-0.6; 
    float a = 0.50*(1.0-abs(2.0*noise(p*1.2+vec3(1.0)  +iGlobalTime*0.05)-1.0)); a = a*a*a*a;
    float b = 0.40*(1.0-abs(2.0*noise(p*2.4+vec3(100.0)-iGlobalTime*0.07)-1.0)); b = b*b*b*b;
    float c = 0.13*(1.0-abs(2.0*noise(p*20.+vec3(200.0)+iGlobalTime*0.43)-1.0)); c = c*c*c*c;
    float d = 0.12*(1.0-abs(2.0*noise(p*220.+vec3(500.0)+iGlobalTime*0.53)-1.0)); d = d*d*d*d;
    f = f-a-b-c-d;
    return f;
}

float interior(vec3 p, out int mattype) {
    float f;
    float t =exterior(p, mattype);  
    f = opSubtract(t, -90000.0);
    p.x -= noise(p.zz*0.1+iGlobalTime*0.02) * 3.0;
    p = opRepeat(p, REPEATS);

    // tubes inside the body
    p = rotateZ(0.8+iGlobalTime*0.1)*p;

    for (int t = 0; t < 2; t++) {
        float f1 = length(p.xy-vec2(float(t)*0.3-0.2,0.0))-0.10; 
        float a = 0.40*(1.0-abs(2.0*noise(p*1.2+vec3(1.0)  +iGlobalTime*0.09)-1.0)); a = a*a*a;
        float b = 0.30*(1.0-abs(2.0*noise(p*4.4+vec3(100.0)-iGlobalTime*0.02)-1.0)); b = b*b*b;
        f1 = f1-a-b;
        f= opAdd(f, f1);
    }
    mattype = MATERIAL_INTERIOR; 
    if (abs(f-t)<0.2) mattype = MATERIAL_EXTERIOR;
    return f;
}


float interior(vec3 p) { int unused; return interior(p, unused); }
float exterior(vec3 p) { int unused; return exterior(p, unused); }

struct TraceResult traceInterior(vec3 ro, vec3 rd) {
    int mattype = 0;
    int watermat = 0;
    struct TraceResult traceResult = TraceResult(false, 0.0, mattype);
    float t = 0.02;
    float tmax = MAXT; 
    for (;t < tmax;) {
        float d = interior(ro+rd*t, mattype);
        if (d<0.001) { 
            traceResult = TraceResult(true, t, mattype);
            break;
        }
        t += d;
    }
    traceResult.rayt = t;
    return traceResult;
}

struct TraceResult traceExterior(vec3 ro, vec3 rd) {
    int mattype = 0;
    int watermat = 0;
    struct TraceResult traceResult = TraceResult(false, 0.0, mattype);
    float t = 0.02;
    float tmax = MAXT; 
    for (;t < tmax;) {
        float d = exterior(ro+rd*t, mattype);
        if (d<0.001) { 
            traceResult = TraceResult(true, t, mattype);
            break;
        }
        t += d;
    }
    traceResult.rayt = t;
    return traceResult;
}

vec3 calcInteriorNormal(vec3 p) {
    vec2 eps = vec2(0.001,0.0);
    float x = interior(p+eps.xyy)-interior(p-eps.xyy);
    float y = interior(p+eps.yxy)-interior(p-eps.yxy);
    float z = interior(p+eps.yyx)-interior(p-eps.yyx);
    return normalize(vec3(x,y,z));
}

vec3 calcExteriorNormal(vec3 p) {
    vec2 eps = vec2(0.001,0.0);
    float x = exterior(p+eps.xyy)-exterior(p-eps.xyy);
    float y = exterior(p+eps.yxy)-exterior(p-eps.yxy);
    float z = exterior(p+eps.yyx)-exterior(p-eps.yyx);
    return normalize(vec3(x,y,z));
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

void getMaterial(in int mattype, out vec3 matcolor) {
    matcolor = vec3(0.0);
    vec3 exteriorColor1 = vec3(0.223, 0.050, 0.019);
    vec3 exteriorColor2 = vec3(0.105, 0.117, 0.172);
    vec3 interiorColor1 = vec3(0.521, 0.298, 0.098);
    vec3 interiorColor2 = vec3(0.611, 0.043, 0.815);

    float f = sin(iGlobalTime*0.2)*0.5+0.5;
    if (mattype == MATERIAL_EXTERIOR) { 
        matcolor = mix(exteriorColor1, exteriorColor2, f);
    }
    if (mattype == MATERIAL_INTERIOR) {
        matcolor = mix(interiorColor1, interiorColor2, f);
    }
    
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


    vec3 color = vec3(0.0);

    vec3 rd = normalize(vec3(st, 1.0));
    rd = rotateY(-mouse.x) * rotateX(mouse.y)  * rd;
    rd = normalize(rd);

    vec3 ro = vec3(1.0, 1.0, -7.0); 
    ro += up * 0.10 * normalize(vec3(0.0, 1.0, 0.0));
    ro += forward *1.1*normalize(vec3(0.0, 0.0, 1.0));
    ro += iGlobalTime*0.05*normalize(vec3(0.0, 0.0, 1.0));


    TraceResult exteriorTrace = traceExterior(ro, rd);
    if (exteriorTrace.hit) {
        vec3 exteriorPosition = ro+exteriorTrace.rayt*rd;
        vec3 exteriorNormal = calcExteriorNormal(exteriorPosition);


        // trace inside the thing 
        vec3 interiorStartPosition = exteriorPosition - exteriorNormal * 0.01; 
        vec3 interiorRayDirection = refract(rd, exteriorNormal, REFRETA); 
        TraceResult interiorTrace = traceInterior(interiorStartPosition, interiorRayDirection);

        vec3 interiorColor;
        if (interiorTrace.hit) {
            getMaterial(interiorTrace.materialID, interiorColor);

            vec3 interiorPosition = interiorStartPosition+interiorTrace.rayt*interiorRayDirection;
            vec3 interiorNormal = calcInteriorNormal(interiorPosition);
            // compute lighting inside
            interiorColor = interiorColor*(0.3+phongDiffuseFactor(-SUNDIRECTION, interiorNormal)*1.0
                                              +phongSpecularFactor(-SUNDIRECTION,interiorNormal, interiorRayDirection, 50.0)*1.0
            );

            // exit the interior....
            if (interiorTrace.materialID == MATERIAL_EXTERIOR)
                interiorColor += sky(interiorPosition, interiorRayDirection) * 0.2;
        }

        vec3 exteriorColor; 
        getMaterial(exteriorTrace.materialID, exteriorColor);
        exteriorColor = exteriorColor*(0.3+phongDiffuseFactor(-SUNDIRECTION, exteriorNormal)*1.0 
                                          +phongSpecularFactor(-SUNDIRECTION,exteriorNormal, rd, 50.0)*1.0
        );

        vec3 absorbance = exp(-ABSORBANCE*ABSORBANCE_SCALE*interiorTrace.rayt);
        float fresnel = pow(dot(exteriorNormal, -rd), EXTERIOR_FRESNEL_POW);
        fresnel *= TRANSPARENCY;
        vec3 reflection = sky(ro, reflect(rd, exteriorNormal));
        color = reflection*exteriorColor*(1.0-fresnel) + interiorColor*absorbance*(fresnel);

                
        color = mix(color, FOGCOLOR, fog(exteriorTrace.rayt));
        color = clamp(color, 0.0, 1.0);
    } else {
        color = sky(ro, rd);
    }



    fragColor = vec4(color, 1.0); 
}
