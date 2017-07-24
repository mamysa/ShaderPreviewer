// Trying out some ocean water using sum of sines approach described in GPUgems (https://developer.nvidia.com/gpugems/GPUGems/gpugems_ch01.html)
// Looks more like jello rather than actual water though.
// This whole thing breaks down if camera is under the water surface... 

#version 450
#define PI 3.141592653589793 

const float WORLEYMAXDIST = 1.3; // worley noise max distance
const float MAXLENGTH = 190.0; // maximum ray length


const float FOGHEIGHT = 0.01; // background fog height. 
const float FOGFADEHEIGHT = 0.30; // background fog fade height - fades background fog into actual sky.
const vec3  FOGCOLOR = vec3(0.839, 1, 0.980);
const float FOGDENSITY = 0.010; 

const vec3  OCEAN_COLOR = vec3(0.164, 0.345, 0.454); // water color
const vec3  OCEAN_REFRACTION_COLOR = vec3(0.0, 0.1, 0.8); // Color we fall back to if the ocean floor is beyond the maximum visibility distance.
const vec3  OCEAN_ABSORBANCE = vec3(0.6, 0.25, 0.30);
const float OCEAN_ABSORBANCE_SCALE = 0.09;
const float OCEAN_FRESNEL_POW = 2.0;
const float OCEAN_SPEC_FACTOR = 90.0;
const float N1 = 1.0;  // refractive index 1 - air
const float N2 = 1.32; // refractive index 2 - ocean 

const vec3  SUNDIRECTION = normalize(vec3(1.0, -0.4, -0.9)); //direction of the sunlight
const vec3  SUNCOLOR =vec3(1.0, 0.949, 0.839); // sun color? 
const float SUNINTENSITY = 1.2;

const vec3  SKYCOLOR = vec3(0.513, 0.882, 0.945);
const float SKYCLOUDSCALE = 11.0;

const float CONTRAST = 0.20;
const float BRIGHTNESS = 1.8;
const float GAMMA = 1.2;  // higher => darker; lower => brighter
const float SATURATION = 1.8;

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

float random(in vec2 st) { 
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

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

float noise2D(vec2 uv) {
    vec2 st = 0.1 * uv; 
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

float worley(vec2 uv) {
	vec2 tileCoord = floor(uv);    
    float dist = 90000.0;
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        vec2 currentTile = tileCoord + vec2(x, y);
        vec2 point = currentTile + random(currentTile);
        dist = min(dist, length(point-uv)); 
    }
    dist = clamp(dist, 0.0, WORLEYMAXDIST) / WORLEYMAXDIST;
    return dist;
}

float fbm(vec2 st) {
    float value = 0.0;
    float amplitud = 1.0;
    float frequency = 0.25;
    const float octaves = 3;
    for (int i = 0; i < octaves; i++) {
        value += amplitud * noise2D(st * frequency);
        st *= 2.;
        amplitud *= .10;
        frequency *= 0.5;
    }
    return value;
}

float fbm_ridgesin(vec2 st) {
    float value = 0.0;
    float amplitude = 0.9;
    float frequency = 0.50;
    const int octaves = 3;
    for (int i = 0; i < octaves; i++) {
        float noise = sin(dot(vec2(0.0, 1.0),st)*frequency* 0.9+3.0*noise2D(st*frequency*5.0)); 
        float temp = amplitude*abs(noise); 
        value += pow(temp, 3.0); 
        st *= 2.;
        amplitude *= .50;
        frequency *= 0.8;
    }
    return value;
}

float oceanfloor(vec2 st, out int mattype) {
    // sand 
    float h = 0.0;    
    h = fbm_ridgesin(st*15) * 0.02;
    h += noise2D(st * 1.3) * 4.5;
    // fake seaweed
    float f = (1.0-worley(st * 2.5)) * 0.25;
    f += noise2D(st * 2.0) * 2.0;
    h = max(f, h);
    mattype = 1; 
    if (h == f) mattype = 2;
    return h * 1.1 - 50.0;
}

// wrapper with materialID hidden.
float oceanfloor(vec2 st) { int unused; return oceanfloor(st, unused); }

struct TraceResult traceOceanFloor(vec3 ro, vec3 rd, float dstep) {
    struct TraceResult traceResult = TraceResult(false, 0.0, 0);

    float t = 0.02;
    float tmax = MAXLENGTH;
    for (;t < tmax;) {
        int m = 0;
        vec3 rp = ro + rd * t;
        float h = oceanfloor(rp.xz, m);
        float d = rp.y - h;

        if (d < 0.01) {
            traceResult = TraceResult(true, t, m);
            break;
        }
        t += dstep * d;
    }

    traceResult.rayt = t;
    return traceResult;
}

vec3 getOceanFloorNormal(vec3 rp) {
    vec2 eps = vec2(0.01, 0.0);
    vec3 normal = normalize(vec3( 
        oceanfloor(rp.xz - eps.xy).x - oceanfloor(rp.xz + eps.xy).x,
        2.0 * eps.x, 
        oceanfloor(rp.xz - eps.yx).x - oceanfloor(rp.xz + eps.yx).x 
    ));
    return normal;
}

/// WATER SURFACE 
// 1- amplitude, 2 - frequency, 3 - speed;
float oceanSurface(vec2 st, out int mattype) {
    float time = iGlobalTime;
    float h = 3.0;
    h += 0.35*sin(dot(vec2( 0.0, 1.0),st)*0.5+5.0*noise2D(st*0.1)+time*0.50); 
    h += 0.45*sin(dot(vec2(-1.4, 0.8),st)*0.1+5.0*noise2D(st*0.5)+time*0.45); 
    h += 0.15*sin(dot(vec2( 0.4, 1.0),st)*0.4+3.0*noise2D(st*3.0)+time*0.60); 
    h += 0.09*sin(dot(vec2(-0.3, 1.0),st)*0.9+3.0*noise2D(st*4.0)+time*0.92); 
    h+=fbm(st*50.0+time*4.0)*0.06;
    mattype = 3;
    return h;
}

// wrapper with materialID hidden.
float oceanSurface(vec2 st) { int unused; return oceanSurface(st, unused); }

struct TraceResult traceOceanSurface(vec3 ro, vec3 rd, float dstep) {
    struct TraceResult traceResult = TraceResult(false, 0.0, 0);

    float t = 0.02;
    float tmax = MAXLENGTH;
    for (;t < tmax;) {
        int m = 0;
        vec3 rp = ro + rd * t;
        float h = oceanSurface(rp.xz, m);
        float d = rp.y - h;

        if (d < 0.01) {
            traceResult = TraceResult(true, t, m);
            break;
        }
        t += dstep * d;
    }

    traceResult.rayt = t;
    return traceResult;
}

vec3 getOceanSurfaceNoraml(vec3 rp) {
    vec2 eps = vec2(0.01, 0.0);
    vec3 normal = normalize(vec3( 
        oceanSurface(rp.xz - eps.xy).x - oceanSurface(rp.xz + eps.xy).x,
        2.0 * eps.x, 
        oceanSurface(rp.xz - eps.yx).x - oceanSurface(rp.xz + eps.yx).x 
    ));

    return normal;
}

float bad_ao(vec3 n) {
    return abs(dot(n, vec3(0.0, 1.0, 0.0))); 
}

float fog(float dist) {
    return  1.0 - 1.0/exp(pow(dist*FOGDENSITY, 2.0));
}

// borrowed this from a certain thread on pouet
vec3 postprocess(vec3 color) {
    color = color * BRIGHTNESS;
    color = pow(color, vec3(GAMMA));
    color = color * 0.5 + CONTRAST * 0.5;
    float luminance = dot(vec3(0.2126, 0.7152, 0.0722), color);
    color = luminance + (color - luminance) * SATURATION;
    return clamp(color, 0.0, 1.0);
}

vec3 sky(vec3 ro, vec3 rd) {
    vec3 color = SKYCOLOR; 
    float time = iGlobalTime;
    // borrowed it from one of iq's demos, hopefully no one will notice...
    color += smoothstep(0.3, 1.0, fbm(rd.xz*SKYCLOUDSCALE/ rd.y + time*0.15));
    float d = dot(-SUNDIRECTION, rd); // sun??
    if (d > 0.0)          
        color = mix(color, vec3(1.0), pow(d, 150.0));
    if (rd.y < FOGFADEHEIGHT)     
        color = mix(FOGCOLOR, color, (rd.y-FOGHEIGHT)/(FOGFADEHEIGHT-FOGHEIGHT));
    if (rd.y < FOGHEIGHT) 
        color = FOGCOLOR;
    return clamp(color, 0.0, 1.0);
}

float phongAmbientFactor(void) {
    return 0.1;
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

void getMaterial(int mattype, vec3 rp, out vec3 color1, out vec3 color2, 
                                       out float smin, out float smax,
                                       out float vmin, out float vmax) {
    // texture slope fade constants
    // anything below smin slope uses grass texture, anything above uses rock texture,
    // values between are lerped.
    smin = 0.02;  
    smax = 0.09;

    // terrain detail texture
    vmin = 0.95;
    vmax = 1.0;

    // default colors
    color1 = vec3(1.0, 0.0, 0.0);
    color2 = vec3(1.0, 0.0, 0.0);

    if (mattype == 1) {
        color1 = vec3(1, 0.937, 0.678); //  snow color;
        color2 = vec3(0.900, 0.815, 0.539); // rock 
    }
    if (mattype == 2) {
        color1 = vec3(0, 0.301, 0.047); //  tree base;
        color2 = vec3(0.341, 0.784, 0.317); //   tree top;
        color1 = color1 + vec3(noise2D(rp.xz * 9.0),  noise2D(rp.zx * 2.0), noise2D(rp.zx)) * 0.10;
        color2 = color2 + vec3(noise2D(rp.xz * 5.0),  noise2D(rp.zx * 9.0), noise2D(rp.zx)) * 0.10;
        smin = 0.09;
        smax = 0.14;
        vmin = 0.2;
        vmax = 1.0;
    }
    if (mattype == 3) {
        color1 = OCEAN_COLOR;
    }
}

void main(void) {
    vec3 color1, color2;
    float smin, smax, vmin, vmax;
    vec2 eps = vec2(0.1, 0.0);

    vec2 st = fragTexCoord;
    float finv = tan(90.0 * 0.5 * PI / 180.0);
    float aspect = resolution.x / resolution.y;
    st.x = st.x * aspect;
    st = (st - vec2(aspect * 0.5, .5)) * finv;

    vec3 rd = normalize(vec3(st, 1.0));
    rd = rotateY(-mouse.x+0.785398) * rotateX(mouse.y)  * rd;
    rd = normalize(rd);
 
    vec3 ro = vec3(0.0, 10.0, 0.0); 
    ro += up * normalize(vec3(0.0, 1.0, 0.0));
    ro += forward *  normalize(vec3(-1.0, 0.0, 1.0));
    ro += iGlobalTime * 0.2 * normalize(vec3(-1.0, 0.0, 1.0));

    // trace ocean surface
    struct TraceResult oceanSurfaceTrace = traceOceanSurface(ro, rd, 0.4);
    vec3 rp = ro + oceanSurfaceTrace.rayt * rd;
    vec3 n = getOceanSurfaceNoraml(rp);
    float saf = phongAmbientFactor();
    float sdf = phongDiffuseFactor(-SUNDIRECTION, n);
    float ssf = phongSpecularFactor(-SUNDIRECTION, n, rd, OCEAN_SPEC_FACTOR);

    // compute fresnel factor for the water surface 
    float fr = pow(dot(n, -rd), OCEAN_FRESNEL_POW);

    // compute reflection color;
    vec3 refl_color = OCEAN_COLOR * 0.3;
    vec3 refl_ro = rp + eps.yxy;
    vec3 refl_rd = reflect(rd,n);
    TraceResult reflectionTraceResult = traceOceanSurface(refl_ro,refl_rd,0.5);
    if (reflectionTraceResult.hit) refl_color = sky(refl_ro, refl_rd) * 0.2;
    
    // compute refraction
    vec3 refr_color = OCEAN_REFRACTION_COLOR * 0.1;
    float eta = N1/N2;
    vec3 refr_ro = rp - eps.yxy;
    vec3 refr_rd = refract(rd, n, eta);
    TraceResult refractionTraceResult = traceOceanFloor(refr_ro, refr_rd, 0.2);
    if (refractionTraceResult.hit) {
        // compute lighting of the the seafloor
        vec3 refr_rp = refr_ro + refractionTraceResult.rayt * refr_rd;
        vec3 refr_n = getOceanFloorNormal(refr_rp);
        float faf = phongAmbientFactor();
        float fdf = phongDiffuseFactor(-SUNDIRECTION, refr_n);
        getMaterial(refractionTraceResult.materialID, refr_rp, color1, color2, smin, smax, vmin, vmax);

        float slopefactor = 1.0 - abs(dot(refr_n, vec3(0.0, 1.0, 0.0)));
        slopefactor = clamp(slopefactor, smin, smax);
        slopefactor = (slopefactor-smin) / (smax - smin);
        refr_color = mix(color1, color2, slopefactor);

        float ao = bad_ao(refr_n);
        vec3 absorbance = exp(-OCEAN_ABSORBANCE*OCEAN_ABSORBANCE_SCALE*refractionTraceResult.rayt);

        refr_color = refr_color*faf*ao + refr_color*fdf;
        refr_color *= absorbance;
        refr_color *= SUNCOLOR * SUNINTENSITY;
        refr_color = clamp(refr_color, 0.0, 1.0);
    } 
    refl_color = refr_color*fr+(1.0-fr)*refl_color;

    getMaterial(oceanSurfaceTrace.materialID, rp, color1, color2, smin, smax, vmin, vmax);

    vec3 color = clamp(color1*saf+color1*sdf+ssf+refl_color, 0.0, 1.0);
    color *= SUNCOLOR * SUNINTENSITY;
    color = mix(color, FOGCOLOR, fog(oceanSurfaceTrace.rayt));
    if (!oceanSurfaceTrace.hit)  color = sky(ro, rd); 
    color = postprocess(color);
    color = clamp(color, 0.0, 1.0);
    fragColor = vec4(color, 1.0);
}