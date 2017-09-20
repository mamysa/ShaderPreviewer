// Trying some more clouds. These are really expensive.
// Some artifacts when entering the cloud volume - samples on the horizon become 
// slightly darker than they are supposed to be. Also some banding.
// I am guessing it could be improved by taking variable number of samples or 
//something like that. Will take a look again once I have more free time. 
// I am also going to add some explanation about what I am doing later... so that
// I don't forget myself. 

#version 450
#define PI 3.141592653589793 

const float MAXT = 180.0f;
const float FOGHEIGHT = 0.40; // background fog height. 
const float FOGFADEHEIGHT = 0.60; // background fog fade height - fades background fog into actual sky.
const vec3  FOGCOLOR = vec3(1, 0.878, 0.721);
const float FOGDENSITY = 0.0080; 
const float FOGPOWER = 3.5;

const vec3  SUNDIRECTION = normalize(vec3(0.0, -1.00, -1.0)); //direction of the sunlight
const vec3  SUNCOLOR =vec3(0.949, 0.741, 0.470); // sun color? 
const vec3  SKYCOLOR = vec3(1, 0.760, 0.521);

// colors for scene objects...
const vec3 TILE1_COLOR  = vec3(0.6);
const vec3 TILE2_COLOR  = vec3(0.8);

const vec3 CLOUD_AMBIENT_COLOR = vec3(1.0, 0.833, 0.856); 

// number of cloud samples. When sampling light, less samples are used, and that 
// number is just numsteps*light_multiplier. 
const float CLOUDTRACE_NUMSTEPS = 40.0; 
const float CLOUDTRACE_LIGHT_MULTIPLIER = 0.25; 

const float CLOUD_EXTINCTION = 1.20;
const float CLOUD_SCATTERING = 0.15;

// cloud low and high planes. LO < HI.
const float CLOUD_Y_LO = -6.0;
const float CLOUD_Y_HI =  0.0;

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
    float ret = clamp(2.0*fg-1.0, -1.0, 1.0);
    return (ret < 0.0) ? 0.0: ret;
}


float fbm(vec3 p) {
    float f = 0.0;
    float a = 0.5;
    f += a*noise(p); p*=1.25; a *= 0.5;
    f += a*noise(p); p*=2.00; a *= 0.5;
    f += a*noise(p); p*=5.50; a *= 0.5;
    f += a*noise(p); 
    return f;
}

// cloud density func
float getCloudDensity(vec3 p) {
    float f = fbm(p*0.05 + iGlobalTime *0.01);
    float a = (p.y >= CLOUD_Y_LO && p.y <= CLOUD_Y_HI) ? 1.0: 0.0;
    return a*f;
}


// horizonal plane intersection test. 
float hPlaneIntersection(vec3 ro, vec3 rd, float y) {
    vec3 n = vec3(0.0, 1.0, 0.0);
    vec3 p0 = vec3(0.0, y, 0.0);

    float denom = dot(rd, n); 
    if (denom == 0.0) return -1.0; // parallel to the plane 
    float num = dot(p0-ro, n);
    return num/denom;
}

float sdPlane(vec3 p, vec4 n) { return dot(p, n.xyz) - n.w; }

// map TODO maybe something nicer... performance permitting.
float map(vec3 p) {
    float pl = sdPlane(p, vec4(0.0, 1.0, 0.0, -20.0));
    return pl;
}

struct TraceResult trace(vec3 ro, vec3 rd) {
    struct TraceResult traceResult = TraceResult(false, 0.0);
    float t = 0.02; float tmax = MAXT; 

    for (; t < MAXT; ) {
        vec3 rp = ro + rd * t;
        bool hitFogVolumeTemp = false;
        float mapTrace = map(rp);
        if (mapTrace.x<0.01) {
            traceResult.hit = true;
            break;
        }

        t += mapTrace.x; 
    }

    traceResult.rayt = t;
    return traceResult;
}

void cloudCalcLight(vec3 ro, vec3 rd, float tmax, float numsamples, inout vec3 lightAmb, inout vec3 lightSun) {
    const float stepsize = tmax/numsamples;
    float eps = 0.0005;
    float transmittance = 1.0;

    for (float j = eps; j <= tmax; j += stepsize) {
        vec3 rp = ro+j*rd;
        float density = getCloudDensity(rp);
        if (density > 0.0) {
            float extinctionCoeff = CLOUD_EXTINCTION * density;
            transmittance *= exp(-extinctionCoeff*stepsize);
            lightAmb += transmittance * CLOUD_AMBIENT_COLOR; 
            lightSun += transmittance * SUNCOLOR;
        }
    }
}

vec4 traceCloud3(vec3 ro, vec3 rd, vec3 ld, float tmin, float tmax, float numsamples) {
    vec3  Le = vec3(0.0); // light quantity 
    float transmittance = 1.0; // initial tranmittance
    float sunphase = 3.0/4.0*(dot(rd,ld)*dot(rd,ld)+0.5);
    float ambphase = 0.4;

    const float stepsize = (tmax-tmin) / numsamples;
    for (float i = tmin+0.02; i <= tmax; i+=stepsize) {
        vec3 rp = ro+i*rd;
        float density =  getCloudDensity(rp); 
        if (density > 0.0) {
            float extinctionCoeff = CLOUD_EXTINCTION * density;
            float scatteringCoeff = CLOUD_SCATTERING * density;
            transmittance = transmittance * exp(-extinctionCoeff*stepsize);

            // for now we assume that sunlight comes from above and not below...
            // tmin for calcLight is always 0, can ignore it... :P
            float hi = hPlaneIntersection(rp, ld, CLOUD_Y_HI);
            vec3 lightSunIn = vec3(0.0); 
            vec3 lightAmbIn = vec3(0.0);
            cloudCalcLight(rp+ld*hi, -ld, hi, ceil(numsamples*CLOUDTRACE_LIGHT_MULTIPLIER), lightAmbIn, lightSunIn);

            vec3 scattering = scatteringCoeff * (sunphase*lightSunIn + ambphase*lightAmbIn);
            Le += transmittance * scattering * stepsize;
        }
    }

    return vec4(Le, transmittance);
}

vec3 calcNormal(vec3 p) {
    vec2 eps = vec2(0.001,0.0);
    float x = map(p+eps.xyy)-map(p-eps.xyy);
    float y = map(p+eps.yxy)-map(p-eps.yxy);
    float z = map(p+eps.yyx)-map(p-eps.yyx);
    return normalize(vec3(x,y,z));
}

//Shadow function. I have no idea what I am doing but it looks okay. 
float traceShadow(vec3 ro, vec3 rd) {
    float numsteps = 15.0;
    float stepsize = 2.0;
    float d = 0.0;
    for (float i = 0.0; i < numsteps *stepsize; i+=stepsize) {
        d += clamp(getCloudDensity(ro+i*rd),0.0,1.0) * 0.5;
    }
    return 1.0-clamp(d, 0.0,1.0);
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

    float t = MAXT;
    TraceResult traceResult = trace(ro, rd);
    vec3 rp = ro;


    if (traceResult.hit) {
        t = traceResult.rayt;
        rp = ro+traceResult.rayt*rd;
        vec3 n = calcNormal(rp);
        vec3 tiles = (sin(rp.z) + sin(rp.x) <= 0.0) ? TILE1_COLOR : TILE2_COLOR;
        vec3 sp = rp + 0.1*n;
        float ph = phongDiffuseFactor(-SUNDIRECTION, n) 
                 + phongSpecularFactor(-SUNDIRECTION, n, rd, 10.0);
        color = 0.2+SUNCOLOR*ph*traceShadow(sp, -SUNDIRECTION);
        color = tiles * color;

        float fg = fog(t, FOGDENSITY, FOGPOWER);
        color = mix(color, FOGCOLOR, fg);
    }

    float lo = hPlaneIntersection(ro, rd, CLOUD_Y_LO);
    float hi = hPlaneIntersection(ro, rd, CLOUD_Y_HI);

    // volumes boundary...
    float near = min(lo,hi); near = (near >= 0.0) ? near : 0.0;
    float far  = max(lo,hi);  far = (far >= MAXT*1.5) ? MAXT * 1.0: far;

    if (far > 0.0 && near < MAXT*1.5 && far < MAXT*1.5) {
        float numsamples = CLOUDTRACE_NUMSTEPS; 
        vec4 cloudTrace = traceCloud3(ro, rd, -SUNDIRECTION, near, far, numsamples);
        color = cloudTrace.xyz + cloudTrace.w * color;
        float fg = fog(near, FOGDENSITY, FOGPOWER);
        color = mix(color, FOGCOLOR, fg);
    }

    
    color = clamp(color, 0.0, 1.0);
    fragColor = vec4(color, 1.0); 
}