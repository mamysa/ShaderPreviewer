// Trying out some of this cool terrain stuff. A bit too slow sadly.
#version 450
#define PI 3.141592653589793 

const float MAXT = 100.0f;
const float WORMHOLE_EPS = 0.01;
const float STARS_EPS = 0.0001;

const float FOGHEIGHT = 0.01; // background fog height. 
const float FOGFADEHEIGHT = 0.30; // background fog fade height - fades background fog into actual sky.
const vec3  FOGCOLOR = vec3(0.839, 1, 0.980);
const float FOGDENSITY = 0.012; 

const vec3  SUNDIRECTION = normalize(vec3(-1.0, -0.6, -0.9)); //direction of the sunlight
const vec3  SUNCOLOR =vec3(0.945, 0.960, 0.880); // sun color? 
const vec3  SKYCOLOR = vec3(0.513, 0.882, 0.945);




in vec2 fragTexCoord;
out vec4 fragColor;
uniform vec3 resolution;

uniform vec3 viewPosition;
uniform mat3 viewMatrix;
uniform float iGlobalTime;
uniform sampler2D iChannel0;

struct TraceResult {
    bool hit;
    float rayt;
    vec3 color; 
};

vec3 triplanarTexture(vec3 p, vec3 n, sampler2D tex) {
    n = abs(n); 
    n = n*(1.0/(n.x+n.y+n.z));
    return  n.x*texture2D(tex, p.yz).xyz + 
            n.y*texture2D(tex, p.xz).xyz +
            n.z*texture2D(tex, p.xy).xyz;
}

vec3 doBumpMap(in vec3 p, in vec3 n, float bf, sampler2D tex){
    const vec2 e = vec2(0.001, 0);

    vec3 b1 = triplanarTexture(p-e.xyy, n, tex);
    vec3 b2 = triplanarTexture(p-e.yxy, n, tex);
    vec3 b3 = triplanarTexture(p-e.yyx, n, tex);
    vec3 b  = triplanarTexture(p, n, tex);
    
    float c1 = (b1.x + b1.y + b1.z)/3.0;
    float c2 = (b2.x + b2.y + b2.z)/3.0;
    float c3 = (b3.x + b3.y + b3.z)/3.0;
    float c = (b.x + b.y + b.z)/3.0;
    
    vec3 g = vec3(c1, c2, c3);
    g = g - c; g =  g/e.x;
    return normalize(n + g*bf);
}



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

const mat3 rm1 = rotateZ( 0.015*iGlobalTime);
const mat3 rm2 = rotateZ( 0.032*iGlobalTime);

float fbm(vec2 st, mat3 rotmat) {
    float value = 0.0;
    float amplitud = 1.0;
    float frequency = 0.2;
    const float octaves = 3;
    for (int i = 0; i < octaves; i++) {
        st = vec2(rotmat*vec3(st,0.0));
        float f = amplitud*(1.0-abs(2.0*noise(st*frequency)-1.0));
        f=f*f*f;
        value += f;
        st *= 3.5 + vec2(2.0, 2.0);
        amplitud *= .75;
        frequency *= 0.4;
    }
    value = clamp(value, 0.0, 1.0);
    return value;
}

// GEOMETRY FUNCTIONS
float bgLightingBolts(in vec2 st) {
    float f1 = fbm(st*90.0, rm1); 
    float f = fbm(st*50.0 - vec2(f1) , rm2);
    return f;
}


float sdSpheroid(vec3 p, vec3 s) {
    return (length(p/s) - 1.0)  * min(s.x, min(s.y, s.z));
}

float sdBox(vec3 p, vec3 b) {
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,vec3(0.0)));
}

float udRoundBox( vec3 p, vec3 b, float r ) {
  return length(max(abs(p)-b,0.0))-r;
}

float sdPlane(vec3 p, vec4 n) {
    return dot(p, n.xyz) - n.w;
}

float udPlane(vec3 p, vec4 n) {
    return abs(dot(p, n.xyz) - n.w);
}

float sdCylinder( vec3 p, vec3 c ) {
  return length(p.xz-c.xy)-c.z;
}

float sdCone( vec3 p, vec2 c ) {
    float q = length(p.xz);
    return dot(c,vec2(q,p.y));
}

float sdHexPrism( vec3 p, vec2 h ) {
    vec3 q = abs(p);
    return max(q.z-h.y,max((q.x*0.866025+q.y*0.5),q.y)-h.x);
}

// polynomial smooth min (k = 0.1);
float smin( float a, float b, float k ) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float opAdd(float d1, float d2) {
    return min(d1, d2);
}

float opAddSmooth( float a, float b, float k ) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float opSubtract( float d1, float d2 ) {
    return max(-d1,d2);
}

float opIntersect(float d1, float d2) {
    return max(d1,d2);
}

vec4 opAdd(vec4 a, vec4 b) {
    vec4 f;
    f.x = opAdd(a.x, b.x);
    f.yzw = (f.x == a.x) ? a.yzw : b.yzw;
    return f; 
}

vec3 opRepeat(vec3 p, vec3 c) {
    return mod(p,c)-0.5*c;
}


float opMod1(inout float p, float size)  {
	float halfsize = size*0.5;
	float c = floor((p + halfsize)/size);
	p = mod(p + halfsize, size) - halfsize;
	return c;
}

vec3 pMod3(inout vec3 p, vec3 size) {
	vec3 c = floor((p + size*0.5)/size);
	p = mod(p + size*0.5, size) - size*0.5;
	return c;
}

vec3 opRepeatSectorID(vec3 p, vec3 c) {
    return floor(p/c);
}

void pRepeatPolar(inout vec2 p, float repetitions) {
	float angle = 2*PI/repetitions;
	float a = atan(p.y, p.x) + angle/2.;
	float r = length(p);
	a = mod(a,angle) - angle/2.;
    p = vec2(cos(a), sin(a))*r;
}



float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.1;

    const float octaves = 4;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p*frequency);
        p *= 2.;
        amplitude *= 0.3;
        frequency *= 1.4;
    }
    return value;
}

float fbm(vec3 p, int oct) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 0.9;

    for (int i = 0; i < oct; i++) {
        value += amplitude * noise(p*frequency);
        p *= 2.;
        amplitude *= 0.3;
        frequency *= 1.4;
    }
    return value;
}

float fbm(vec2 p, int oct) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 0.9;

    for (int i = 0; i < oct; i++) {
        value += amplitude * noise(p*frequency);
        p *= 2.;
        amplitude *= 0.3;
        frequency *= 1.4;
    }
    return value;
}




vec4 walltrim(vec3 p) {
    float rbound = sdPlane(p, vec4(-1.0, 0.0, 0.0, -2.0));
    float lbound = sdPlane(p, vec4( 1.0, 0.0, 0.0, -7.0));
    vec3 p1 = rotateZ(1.16)*p;
    p1 = p1+vec3(1.13, 0.57, 0.0)  ;

    vec3 p2 = rotateZ(0.8)*p;
    p2 = p2 + vec3(7.0, 1.45, 0.0);

    vec3 p3 = p + vec3(0.0, 10.0, 0.0);
    float f1 = sdBox(p, vec3(2.0, 4.0, 0.5));
    float f2 = sdBox(p1, vec3(4.0, 4.0, 0.5));

    float f3 = sdBox(p2, vec3(3.0, 3.0, 0.5));
    float f4 = sdBox(p3, vec3(7.0, 5.0, 0.5));

    float f = opAdd(f1, f2);
    f = opAdd(f, f4);
    f = opAdd(f, f3 );
    f = opSubtract(rbound, f);
    f = opSubtract(lbound, f);

    return vec4(f, vec3(0.5));
}

float weirdSpikyThing(vec3 p, float spikiness, float smoothness) {
    float f = 900.0;

    vec2 s = normalize(vec2(spikiness, 0.5));


    float numY = 6.0;
    float stepY = 2.0*PI/numY;

    for (float z = stepY; z <= 2.0*PI; z += stepY) 
    for (float x = stepY; x <= 2.0*PI; x += stepY) {
        vec3 q = p;
        q = rotateZ(z) * q;
        q = rotateX(x) * q;
        float t = sdPlane(q, vec4(0.0, 1.0, 0.0, 0.0));
        float c  = sdCone(q-vec3(0.0, 1.0, 0.0), s);
        c  = opSubtract(t, c);
        f = smin(c, f, smoothness);
    }
    return f;
}

float pipe(vec3 p, float depth, float len) {
    float f = sdCylinder(p, vec3(0.0, 0.0, depth));

    float pl1 = sdPlane(p, vec4(0.0,  1.0, 0.0, -len));
    float pl2 = sdPlane(p, vec4(0.0, -1.0, 0.0, -len));
    f = opSubtract(pl1, f);
    f = opSubtract(pl2, f);

    return f;
}


vec4 cross(vec3 p) {
    return vec4( opAdd(
        udRoundBox(p, vec3(30.0, 1.0, 20.0), 0.1), 
        udRoundBox(p, vec3(20.0, 1.0, 30.0), 0.1)), vec3(0.4));
}


vec4 trim_segment(vec3 p) {
    p.z -= 0.75;
    float f = 1.0;
    float w0 = udRoundBox(p, vec3(0.5, 9.0, 2.5), 0.2);
    f = opAdd(f, w0);

    vec3 p1 = p;
    p1 -= vec3(0.0, 1.5, 0.0);
    p1 = rotateX(-PI*0.15)*p1;

    vec3 p2 = p;
    p2 -= vec3(0.0, 5.0, 1.25);
    
    float w1 = udRoundBox(p1, vec3(0.5, 4.0, 2.5), 0.2);
    float w2 = udRoundBox(p2, vec3(0.5, 2.1, 4.5), 0.2);
    f = opAdd(f, w1);
    f = opAdd(f, w2);

    return vec4(f, vec3(0.4));
}

vec4 trim(vec3 p) {
    vec4 trl = trim_segment(p - vec3(1.5, 0.0, 0.0));
    vec4 trr = trim_segment(p + vec3(1.5, 0.0, 0.0));

    vec4 lightp = vec4(sdBox(p, vec3(0.8, 9.0, 1.5)), vec3(1.0, 0.0, 0.0)); 
    return opAdd(lightp, opAdd(trl,trr));
}


vec4 lightcap(vec3 p) {
    float f = MAXT;
    vec3 p1 = p; p1.x += 0.7;
    vec3 p2 = p; p2.x += 0.7;
    p1 = rotateY( 0.25)*p1;
    p2 = rotateY(-0.25)*p2;
    float c0 = udRoundBox(p1, vec3(0.5, 0.72, 2.0), 0.1);
    float c1 = udRoundBox(p2, vec3(0.5, 0.72, 2.0), 0.1);
    f = opAdd(f, c0);
    f = opAdd(f, c1);
    return vec4(f, vec3(0.4));
}

vec4 trim2(vec3 p) {

    float w0 = udRoundBox(p-vec3(0.0,0.0,0.6), vec3(7.9, 6.9, 0.2), 0.1);
    float w1 = udRoundBox(p+vec3(0.0,0.0,0.6), vec3(7.9, 6.9, 0.2), 0.1);
    float f = opAdd(w0, w1);

    float c1 = sdCylinder(p, vec3(-7.4, 0.0, 0.5));
    float c2 = sdCylinder(p, vec3(+7.4, 0.0, 0.5));
    //c = opSubtract(sdPlane(p, vec4(0.0,  1.0, 0.0, -9.5)), c);
    f = opAdd(f, c1);
    f = opAdd(f, c2);

    // trim caps
    vec3 p0 = p; p0.x += 0.0;
    #if 0
    vec3 p1 = p; p1.x += 0.75;
    vec3 p2 = p; p2.x += 0.75;
    p1 = rotateY( 0.25)*p1;
    p2 = rotateY(-0.25)*p2;
    float c0 = udRoundBox(p1, vec3(0.5, 0.8, 2.0), 0.1);
    float c1 = udRoundBox(p2, vec3(0.5, 0.8, 2.0), 0.1);
    f = opAdd(f, c1);
    #endif


    vec3 p1 = p; p1.x += 7.0;
    vec3 p2 = p; p2.x -= 7.0;
    float t1 = lightcap(p1).x;
    float t2 = lightcap(rotateY(PI)*p2).x;
    f = opAdd(f, t1);
    f = opAdd(f, t2);

    


    vec3 pr = rotateX(0.5) * p;
    return vec4(f, vec3(0.4));
}










float platform_corner1(vec3 p) {
    float f = MAXT;

    float b1 = udRoundBox(rotateY(PI*0.5)*p, vec3(2.0, 0.5, 2.0), 0.1);
    float b2 = udRoundBox(rotateY(PI*0.5)*p, vec3(2.0, 0.5, 2.0), 0.1);
    
    float bpl1 = sdPlane(p, vec4(0.0, 0.0,  -1.0, -0.501));
    float bpl2 = sdPlane(p, vec4(-1.0, 0.0,  0.0, -0.501));
    b1 = opSubtract(bpl1, b1);
    b2 = opSubtract(bpl2, b2);

    //corner
    float c = udRoundBox(rotateY(PI*0.25)*p-vec3(0.0, 0.0, 0.245), vec3(1.5, 0.50, 1.5), 0.1);
    float cpl1 = sdPlane(p, vec4(0.0, 0.0,  1.0, 0.496));
    float cpl2 = sdPlane(p, vec4(1.0, 0.0,  0.0, 0.496));
    c = opSubtract(cpl1, c);
    c = opSubtract(cpl2, c);
    f = opAdd(b1, f);
    f = opAdd(b2, f);
    f = opAdd(c, f);
    return f;
}

vec4 wall(vec3 p) {
    vec4 f = vec4(MAXT);
    int numsegs = 3;

    float HPI = PI*0.5;

    for (int i = 0; i < numsegs; i++) {
        vec3  q = p + vec3(0.0, 2.0 * (float(i) - float(numsegs)*0.5) + 1.0, 0.0);
        float w = sdBox(q, vec3(7.5, 0.8, (mod(i,2) == 0) ? 1.75 : 1.6));

        vec3  r = rotateZ(0.8) * p;
        vec3  c = ( mod(i,2) == 0 ) ? vec3(0.4) : 
            mix(vec3(0.8), vec3(0.0), floor(sin(r.x*3.2))  * 0.5 + 0.5);
        f = opAdd(f, vec4(w, c));
    }

    return f;
}


float platform(vec3 p) {
    vec3 d1 = vec3(8.0, 0.0, 1.5);
    vec3 d2 = vec3( d1.x, d1.y, -d1.z);
    vec3 d3 = vec3(-d1.x, d1.y, -d1.z);
    vec3 d4 = vec3(-d1.x, d1.y,  d1.z);
    mat3 m1 = rotateY(PI*0.5);
    mat3 m2 = rotateY(PI);
    mat3 m3 = rotateY(PI*1.5);

    float f  = udRoundBox(p, vec3(6.0, 0.5, 3.5), 0.1);
    float c1 = platform_corner1(p-d1);
    float c2 = platform_corner1(m1*(p-d2));
    float c3 = platform_corner1(m2*(p-d3));
    float c4 = platform_corner1(m3*(p-d4));
    f = opAdd(f, c1);
    f = opAdd(f, c2);
    f = opAdd(f, c3);
    f = opAdd(f, c4);
    return f;
}


vec4 column2(vec3 p) {
    vec4 f = vec4(MAXT, vec3(0.5));
    //f.x = platform(p);



    vec3 pr = opRepeat(p, vec3(0.0, 7.2, 0.0));
    float wb = sdBox(p, vec3(10.0, 7.0, 10.0));




    vec4 w2 = wall(pr);
    w2.x = opSubtract(-wb, w2.x);
    
    vec4 t1 = vec4(udRoundBox(rotateY(+0.05)*p, vec3(8.0, 0.5, 2.1), 0.1), vec4(0.5));
    vec4 t2 = vec4(udRoundBox(rotateY(-0.05)*p, vec3(8.0, 0.5, 2.1), 0.1), vec4(0.5));
    f = opAdd(f, w2);
    f = opAdd(f, t1);
    f = opAdd(f, t2);

    vec4 filler = vec4(sdBox(p, vec3(7.7, 7.0, 1.61)), vec3(0.5));

    f = opAdd(f, filler);
    
    //vec4 l1 = trim2(rotateY(PI)*(p-vec3(7.5, 0.0, 0.0)));
    vec4 l2 = trim2((p-vec3(0.0, 0.0, 0.0)));
    f = opAdd(f, l2);

#if 0
    vec3 p1 = p - vec3(12.0, -3.0, 0.0);
    p1 = rotateX(PI*0.5) * p1;
    p1 = p1 / vec3(1.0, 6.5, 1.0);

    float c1 = platform_corner1(p1);
    f.x = opAdd(f.x, c1);
    #endif



    return f;

}


vec4 bendy_wall(vec3 p) {
    float c = cos(0.1*p.y);
    float s = sin(0.1*p.y);
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(m*p.xy,p.z);

    //q -= vec3(2.3, 18.0, 0.0);
    //q.y -= 8.0;

    q = rotateX(PI*0.5)*q;
    q = rotateY(PI*0.5)*q;
    //return vec4(sdBox(q, vec3(1.0, 6.0, 1.0)), vec3(0.4)) * 0.8;
    vec4 r = wall(q);
    r.x *= 0.5;
    return r;

}

vec4 powercore_supports(vec3 p) {
    vec3 pr = p*vec3(1.006, 1.0, 1.0);
    vec4 w1 = wall(rotateX(PI*0.5)*pr);
    float b = sdBox(p, vec3(7.0, 1.4, 2.0));

    vec4 lt = trim2(rotateZ(PI*0.5)*rotateX(PI*0.5)*p);

    //vec3 pr = rotateZ(PI*0.5)*(p-vec3(0.0, 2.2, 0.0));
    //pr /= vec3(3.0, 1.0, 2.0);
    //vec4 tr = lightcap(pr);
    //tr.x = tr.x;
    w1.x = opAdd(w1.x, b);

    //w1 = opAdd(w1, tr);
    w1 = opAdd(w1, lt);
    return w1;
}


float wall3(vec3 p) {
    vec3 p1 = p;
    p1.y -= 0.25;
    opMod1(p1.y, 0.50);
    float f = udRoundBox(p1, vec3(5.0, 0.15, 1.0), 0.05);



    float t1 = sdBox(rotateZ(PI*0.21)*p, vec3(9.0, 0.5, 1.3));
    float t2 = sdBox(rotateZ(-PI*0.21)*p, vec3(9.0, 0.5, 1.3));

    




    return opAdd(f, opAdd(t1, t2));



}

void pReflect(inout vec3 p, vec4 n) {
    float d = dot(p, n.xyz) - n.w;
    if (d < 0.0)
   		p = p - (2*d)*n.xyz;
 
}

float powercore_podium(vec3 p, float r, float h) {
    vec3 p0 = p;

    pRepeatPolar(p.xz, 8.0);
    p0.y += 0.25*0.5;
    opMod1(p0.y,0.25);
    pRepeatPolar(p0.xz, 8.0);
    p0.x -= r;

    float c = sdCylinder(p0.xzy, vec3(0.0, 0.0, 0.10));
    float b  = sdBox(p, vec3(r+1.0, h, r+1.0));  // bounding volume
    float g = sdPlane(p, vec4(1.0, 0.0, 0.0, r));
    c= opAdd(g, c);
    //c = opAdd(b, c);
    c = opSubtract(-b, c);
    return c;
}


float trim1(vec3 p, float openingWidth, float openingDepth) {
    pReflect(p, vec4(0.0, 1.0, 0.0, -1.0));
    float a = sdBox(p, vec3(openingWidth*2.0, 90000.0, openingDepth-0.05));

    pRepeatPolar(p.xy, 8);
    float b = sdBox(p, vec3(openingWidth, openingWidth*0.5, openingDepth));
    a = opSubtract(b,a);

    p.x -= openingWidth; p.z -= 0.25 * 0.5; opMod1(p.z, 0.25);
    float c = sdCylinder(p, vec3(0.0, 0.0, 0.10));
    c = opSubtract(-b,c);
    a = opAdd(a,c);
    return a;
}

float inftrim2a(vec3 p) {

    //pRepeatPolar(p.xy, 7);
    
    //p = rotateZ(PI)*p;


    //p =rotateZ(0.1)*p;
    opMod1(p.y, 10.0);
    float bound1 = sdBox(p, vec3(2.0, 5.0, 2.0));
    float bound2 = sdBox(p, vec3(2.0, 6.1, 1.0));

    vec3 p1 = p; p1.z -= 0.25*0.5; opMod1(p1.z, 0.25);

    float a = sdCylinder(p1, vec3(0.0, 0.0, 0.10));
    float b = sdPlane(p, vec4(1.0, 0.0, 0.0, 0.0));

    a = opAdd(b,a);
    a = opSubtract(-bound1, a);
    a = opSubtract( bound2, a);

    vec3 p2 = p; p2.x += 0.75; float c2 = opMod1(p2.y, 1.0);
    //p2.x +=abs(c2)*0.05;
    float t1 = udRoundBox(rotateY(0.2)*p2, vec3(0.7, 0.41, 0.8), 0.075);
    float t2 = udRoundBox(rotateY(-0.2)*p2, vec3(0.7, 0.41, 0.85), 0.075);

    a = opAdd(a, opAdd(t1,t2));

    return a;
}

float trim2caps(vec3 p) {
    return 0.0;
}

float infwall2(vec3 p) {
    pReflect(p, vec4(1.0,0.0,0.0,0.0));
    opMod1(p.y,0.75);
    opMod1(p.z,8.0);
    p.x -= 9.5;
    p = rotateZ(PI*0.25) * p;

    float f = udRoundBox(p, vec3(1.0, 1.0, 4.0),0.075);
    float a = udRoundBox(p, vec3(1.4, 1.4, 3.8),0.075);
    f = opAdd(f, a);

    return f;
}




float cro1(vec3 p, float a) {
    vec2 s = vec2(a, 24.0);

    vec3 p1 = p; vec3 p2 = p; vec3 p3 = p;
    pRepeatPolar(p1.xz, 8.0);
    pRepeatPolar(p2.xy, 8.0);
    pRepeatPolar(p3.yz, 8.0);
    float b = sdBox(p1, s.xyx);
    float c = sdBox(p2, s.xxy);
    float d = sdBox(p3, s.yxx);
    float e = sdBox(rotateX(PI*0.5)*p3, s.yxx);
    float cr1 = opAdd(b, opAdd(c, opAdd(d,e)));
    return cr1;
}



vec4 map(vec3 p, vec3 ro, out int mattype) {

    vec4 f = vec4(MAXT, vec3(0.6));
    #if 0
    opMod1(p.y, 1.0);

    pReflect(p, vec4(-1.0, 0.0, 0.0, 0.0));
    pReflect(p, vec4(0.0, 1.0, 0.0, 0.0));

    p.x += 0.25;
    p.y -= 0.25;


    f.x = sdBox(p, vec3(0.25, 0.25, 0.1));


    float a = sdBox(rotateZ(-PI*0.25)*(p-vec3(0.010, 0.265, 0.0)), vec3(0.08,0.025,0.5));
    float b = sdBox((p-vec3(0.25, 0.25, 0.0)), vec3(0.2,0.06,0.5));

    f.x = opSubtract(a, f.x);
    f.x = opSubtract(b, f.x);

    #endif

#if 0
    pReflect(p, vec4(0.0,0.0,-1.0,-4.0));
    vec3 p1 = p; opMod1(p1.z, 16.0);
    vec3 p2 = p; pReflect(p2, vec4(-1.0,0.0,0.0,0.0));
    p2.x += 7.25; p2.z -= 12.0; opMod1(p2.z, 24.0);



    float a = trim1(p1, 5.0, 0.5);
    float b = trim1(p1, 5.75, 1.25);
    float c = infwall2(p);
    f.x = opAdd(a, f.x);
    f.x = opAdd(b, f.x);
    f.x = opAdd(c, f.x);
    //f.x = opAdd(c, f.x);


    //float t = mod(iGlobalTime, 30.0);


    //float a = sdSpheroid(p + vec3(t*0.5,0.0,0.0), vec3(1.0));
    //float b = sdSpheroid(p, vec3(1.0));
    //f.x = opAddSmooth(a, b, 8.7);
    #endif

    float S = 1.2;     
    float Scum = 0;
    int N = 3;
    for (int i = 0; i < 3; i++) {
       p = rotateZ(0.5) * p;
       p = abs(p);
       p.xy += step(p.x, p.y)*(p.yx - p.xy);
       p.xz += step(p.x, p.z)*(p.zx - p.xz);
       p.yz += step(p.y, p.z)*(p.zy - p.yz);
       p = rotateZ(0.1) * p;
       p.z = S*p.z;
       p += vec3(-0.5, 0.4, 1.0);
       Scum += S;
       S *= 0.1;

    }

    f.x = sdBox(p, vec3(1.0)) * 0.1;


    return f;
}



#if 0
vec4 map(vec3 p, vec3 ro, out int mattype) {
    vec4 f = vec4(MAXT, vec3(0.6));


    vec3 pr = p;
    pReflect(pr, vec4(1.0, 0.0, 0.0, 0.0));
    pr.x -= 12.0;




    // brick wall
    vec3 p1 = pr;
    vec3 p2 = pr;
    p2.yz -= 1.0;
    opMod1(p1.z, 2.0);
    opMod1(p1.y, 2.0);
    opMod1(p2.z, 2.0);
    opMod1(p2.y, 2.0);

    float a = udRoundBox(p1, vec3(0.5, 0.430, 0.93), 0.05);    
    float b = udRoundBox(p2, vec3(0.5, 0.430, 0.93), 0.05);    
    f.x = opAdd(a, b);

    //  trim
    vec3 p3 = pr;
    p3.x += 0.75;
    p3.y += 0.25*0.5;
    opMod1(p3.y,0.25);



    vec3 p4 = pr;
    //p4.y -= 0.52;
    opMod1(p4.y,4.0);



    float zz = sdBox(p4, vec3(4.0, 0.45, 90000.0));
    float c = sdCylinder(p3.xzy, vec3(0.0, 0.0, 0.10));
    float g = sdPlane(pr, vec4(-1.0, 0.0, 0.0, 0.75));
    c= opAdd(g, c);
    c = opSubtract(-zz, c);


    //c = opIntersect(c, sdBox(p, vec3(4.0)));
    f.x = opAdd(c, f.x);




    // ver.tical bar trims
    pr += 1.0;
    vec3 p5 = pr;
    vec3 p6 = pr;
    p6.z += 0.25*0.5;
    opMod1(p5.z, 0.25);
    opMod1(p6.z, 25.0);


    float c1 = sdCylinder(p5.xyz, vec3(0.0, 0.0, 0.10));
    float bb2 = sdBox(p6, vec3(12.0, 9000.0, 10.25));

    vec3 p7 = pr;
    p7.z -= 25.0 * 0.5;
    opMod1(p7.z, 25.0);
    float bb3 = sdBox(p7, vec3(12.0, 9000.0, 1.15));



    pr.x -= 0.75; 
    float g2 = sdPlane(pr, vec4(-1.0, 0.0, 0.0, 0.75));
    
    vec3 p8 = pr;
    p8.z -= 25.0 * 0.5;
    p8.y += 1.0;
    p8.x -= 0.45;
    opMod1(p8.y,1.0);
    opMod1(p8.z, 25.0);


    float t1 = udRoundBox(rotateY(0.2)*p8, vec3(0.9, 0.41, 1.0), 0.075);
    float t2 = udRoundBox(rotateY(-0.2)*p8, vec3(0.9, 0.41, 1.0), 0.075);



    c1 = opAdd(g2, c1);
    c1 = opSubtract(bb2, c1);
    f.x = opAdd(c1, f.x);
    
    f.x = opSubtract(bb3, f.x);
    f.x = opAdd(f.x, t1);
    f.x = opAdd(f.x, t2);



    // \_/ shaped trim
    vec3 p9 = pr;
    p9.z -= 25.0 * 0.5;
    p9.y += 1.0;
    p9.x += 0.5;
    opMod1(p9.y, 8.0);
    opMod1(p9.z, 25.0);

    t1 = udRoundBox(rotateY(PI*0.25)*(p9+vec3(-0.1,0.0,-2.0)), vec3(0.9, 0.41, 1.0), 0.075);
    t2 = udRoundBox(rotateY(PI*0.25)*(p9+vec3(-0.1,0.0,2.0)), vec3(0.9, 0.41, 1.0), 0.075);

    float t3 = udRoundBox(p9, vec3(0.9, 0.41, 2.0), 0.075);

    f.x = opAdd(f.x, t1);
    f.x = opAdd(f.x, t2);
    f.x = opAdd(f.x, t3);



    f.x = opAdd(f.x, powercore_podium(p, 9.0, 2.0));

    vec3 p0 = p;
    pRepeatPolar(p0.xz, 6.0);
    p0.x -= 2.0;

    //b = sdBox(p0, vec3(25.0, 0.5, 1.0));
    //f.x  = opAdd(f.x,b);


    return f; 






#if 0 
     this is a pipe test...
    float c = cos(0.012*p.y);
    float s = sin(0.012*p.y);
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(m*p.xy,p.z);
    q.x += sin(q.y) * 0.2;
    q.z += sin(q.y) * 0.2;

    f.x = sdCylinder(q, vec3(0.0, 0.0, 1.0));
    float z = (sin(q.y*6.0)*0.5+0.5)*2.0;
    f.x += (floor(z) + pow(fract(z), 40.0) - 1.0)  * 0.15;
    f.x *= 0.25;
    #endif




    
    //return f;
}
#endif




#if 0
vec4 map(vec3 p, vec3 ro, out int mattype) {
    vec4 f = vec4(MAXT); 

    vec3 pp = p;

    pRepeatPolar(p.xz, 6.0);
    p.x -= 15.0;


    vec3 p1 = p;
    vec3 p2 = p;
    p2.yz -= 1.0;
    opMod1(p1.z, 2.0);
    opMod1(p1.y, 2.0);
    opMod1(p2.z, 2.0);
    opMod1(p2.y, 2.0);



    float a = udRoundBox(p1, vec3(0.5, 0.430, 0.93), 0.05);    
    float b = udRoundBox(p2, vec3(0.5, 0.430, 0.93), 0.05);    
    f.x = opAdd(a, b);


    //  horizontal trim
    vec3 p3 = p;
    p3.x += 0.75;
    p3.y += 0.25*0.5;
    opMod1(p3.y,0.25);

    vec3 p4 = p;
    //p4.y -= 0.52;
    opMod1(p4.y,4.0);



    float zz = sdBox(p4, vec3(4.0, 0.45, 90000.0));
    float c = sdCylinder(p3.xzy, vec3(0.0, 0.0, 0.10));
    float g = sdPlane(p, vec4(-1.0, 0.0, 0.0, 0.75));
    c= opAdd(g, c);
    c = opSubtract(-zz, c);


    //c = opIntersect(c, sdBox(p, vec3(4.0)));
    f.x = opAdd(c, f.x);



    // vertical bar trims
    vec3 p5 = pp;

    p5 = rotateY(PI*0.5)*pp;
    pRepeatPolar(p5.xz, 6);
    p5.x -= 14.25;
    vec3 p6 = p5;

    opMod1(p5.z, 0.25);
    p6.z += 0.25*0.5;
    p6.x -= 0.8;
    opMod1(p6.z, 25.0);



    float c1 = sdCylinder(p5.xyz, vec3(0.0, 0.0, 0.10));
    float bb2 = sdBox(p6, vec3(4.0, 9000.0, 1.75));
    float bb3 = sdBox(p6, vec3(4.0, 9000.0, 0.75));
    float bw = sdPlane(p6, vec4(-1.0, 0.0, 0.0, 0.75));

    c1 = opAdd(c1, bw);

    c1 = opSubtract(-bb2, c1);
    c1 = opSubtract( bb3, c1);



    vec3 p8 = pp;
    p8 = rotateY(PI*0.5)*pp;
    pRepeatPolar(p8.xz, 6);
    p8.x -= 15.5;
    p8.z += 0.1;
    //p8.z -= 25.0 * 0.5;
    //p8.y += 1.0;
    opMod1(p8.y,1.0);
    opMod1(p8.z, 25.0);

    float t1 = udRoundBox(rotateY(0.2)*p8, vec3(0.9, 0.41, 0.5), 0.075);
    float t2 = udRoundBox(rotateY(-0.2)*p8, vec3(0.9, 0.41, 0.5), 0.075);
    c1 = opAdd(c1, t1);
    c1 = opAdd(c1, t2);


    f.x = opAdd(c1, f.x) ; 
     

    // \_/ shaped trims...

    vec3 p9 = rotateY(PI*0.5)*pp;
    pRepeatPolar(p9.xz, 6);

    //p9.z -= 25.0 * 0.5;
    p9.y += 2.0;
    p9.x -= 14.5;
    opMod1(p9.y, 8.0);
    opMod1(p9.z, 25.0);

    float scale = 0.41;
    float offst = -0.1;

    for (int i = 0; i < 2; i++) {
        t1 = udRoundBox(rotateY(PI*0.25)*(p9+vec3(offst,0.0,-2.0)), vec3(0.9, scale, 1.0), 0.075);
        t2 = udRoundBox(rotateY(PI*0.25)*(p9+vec3(offst,0.0,2.0)), vec3(0.9, scale, 1.0), 0.075);
        float t3 = udRoundBox(p9+vec3(offst,0.0,0.0), vec3(0.9, scale, 2.0), 0.075);
        f.x = opAdd(f.x, t1);
        f.x = opAdd(f.x, t2);
        f.x = opAdd(f.x, t3);
        
        scale *= 0.75;
        offst += 0.3;
    }



    #if 0
    f.x = powercore_podium(p, 2.0, 2.0);
    f.x = opAdd(f.x, powercore_podium(p+vec3(0.0,4.0,0.0), 2.3, 2.0));
    f.x = opAdd(f.x, powercore_podium(p+vec3(0.0,8.0,0.0), 2.6, 2.0));


    pRepeatPolar(p.xz, 5.0);
    f.x = opSubtract(sdBox(p, vec3(1.60, 10.0, 2.0)), f.x);
    #endif

    return f; 
}
#endif




vec4 fogmap(vec3 p) {
    p.x = p.x + iGlobalTime*0.1;
    p.z = p.z + sin(iGlobalTime*0.2)*0.2;
    float f = fbm(p*0.4+fbm(p*1.5)) * 1.0; // stained metal ??
    return vec4(f, vec3(1.0));
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



float map(vec3 p) { int unused; return map(p, vec3(0.0), unused).x; }

struct TraceResult trace(vec3 ro, vec3 rd) {
    struct TraceResult traceResult = TraceResult(false, 0.0, vec3(0.0));
    float t = 0.02;
    float tmax = MAXT; 
    int mattype;

    for (; t < MAXT; ) {

        vec3 rp = ro + rd * t;
        vec4 tr = map(rp, ro, mattype);
        if (tr.x<0.010) {
            traceResult = TraceResult(true, t, tr.yzw);
            break;
        }
        t += tr.x;
    }
    traceResult.rayt = t;
    return traceResult;
}

float  calcCloudDensity(vec3 ro, vec3 rd, float maxt) {
    const int numsteps = 25;
    float a = 0.0;
    int t = 1;
    for (; t <= numsteps; t++) {
        float rt = 0.30*float(t); 
        if (rt>maxt) {
            break;
        }
        float d = fogmap(ro+rt*rd).x;
        a += abs( min(d, 0.0) );
    }
    return clamp(a, 0.0, 10.0);
    #if 0
    float t = 0.02;
    float tmax = MAXT; 
    int mattype;

    float a = 0.0;
    for (; t < MAXT; ) {
        vec3 rp = ro + rd * t;
        vec4 tr = map(rp, mattype);
        float d = fogmap(rp).x;
        a += abs( min(d, 0.0) );
        if (tr.x<0.001) {
            break;
        }
        t += tr.x;
    }
    return a;
    #endif
}





vec3 calcNormal(vec3 p) {
    vec2 eps = vec2(0.001,0.0);
    float x = map(p+eps.xyy)-map(p-eps.xyy);
    float y = map(p+eps.yxy)-map(p-eps.yxy);
    float z = map(p+eps.yyx)-map(p-eps.yyx);
    return normalize(vec3(x,y,z));
}

// based on a function in IQ's shadertoy 
//https://www.shadertoy.com/view/Xdl3R4
float calcSurfaceThickness(vec3 ro, vec3 rd, float p) {
    const float TSTEP = 0.9;
    const float TMAX = 2.0;

    const int numsteps = 4;

    float a = 0.0;
    float sca = 1.0;
    for (int t = 1; t <= numsteps; t++) {
        float rt = 0.1*float(t); 
        float d = map(ro+rt*rd);
        a += (rt-min(d,0.0))*sca;
        sca *= 0.9;
    }

    float d = 1.0/float(numsteps);
    return pow(clamp(1.2-d*a, 0.0, 1.0), p);
}

float calcAO(vec3 p, vec3 n) {
    float w = 1.0;
    float accum = 0.0;

    const int maxsteps = 4;
    for (int t = 1; t <= maxsteps; t++) { 
        float rt = 0.1*float(t);
    	accum += w*(rt-map(p+rt*n));   
        w *= 0.8;
    }
    
    return clamp(1.3-accum, 0.0, 1.0);
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

//-----------------------------------------------
// Attenuation function for the pointlight
//-----------------------------------------------
// @k0,k1,k2 - weights
// @r - radius of the pointlight
// @d - distance to fragment
// @return - brightness of the pixel.
float attenuation(float k0, float k1, float k2, float r, float d) {
	float a;
	a = clamp(1.0/(k0+k1*d+k2*d*d), 0.0, 1.0);
	a = (d > r) ? 0.0 : a;
	return a;
}

float fog(float dist, float d) {
    return  1.0 - 1.0/exp(pow(dist*d, 6.0));
}

float calcShading(vec3 ro, vec3 rd, vec3 rp, vec3 n) {
    const float k0 = 0.0;
    const float k1 = -0.52;
    const float k2 = 0.080; 

    const float minL = 30.0; // light fadeout distances...
    const float maxL = 35.0;

    float lighting = 0.0;
    vec3 r = vec3(30.0, 13.2, 20.0); // domain repetition...
    vec3 cameraSectorID = opRepeatSectorID(ro,r); // sector id, to be used for doing lighting patterns...
    for (float z = -2.0; z <= 2.0; z+=1.0) 
    for (float y = -2.0; y <= 2.0; y+=1.0) 
    for (float x = -2.0; x <= 2.0; x+=1.0)  {
        vec3 affectedsector = cameraSectorID+vec3(x,y,z);
        vec3 lightpos = affectedsector*r-vec3( -30.0, 0.0, -9.5);
        vec3 l = lightpos-rp;
        float r = distance(lightpos,ro);
        r = (r >= minL) ? (maxL-clamp(r, minL, maxL))/(maxL-minL) : 1.0;
        lighting += (phongDiffuseFactor(normalize(l),n) + phongSpecularFactor(normalize(l),n, rd, 60.0)) * attenuation(k0, k1, k2, 30.0, length(l)) * r;
    }
    return lighting;
}

void main(void) {
    vec2 st = fragTexCoord;
    float finv = tan(90.0 * 0.5 * PI / 180.0);
    float aspect = resolution.x / resolution.y;
    st.x = st.x * aspect;
    st = (st - vec2(aspect * 0.5, 0.5)) * finv;


    vec3 color;// = FOGCOLOR;





    


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


    float cdf = 1.0;
    TraceResult traceResult = trace(ro, rd);
    if (traceResult.hit) {

        vec3 rp = ro+traceResult.rayt*rd;

        vec3 n = calcNormal(rp);
        float ao = calcAO(rp, n);
        //n = doBumpMap(rp/12//.0 + vec3(0.2), n, 0.03, iChannel0);

        

        // calculate surface thickness 

        //float clouddensity = calcCloudDensity(ro, rd, traceResult.rayt);
        float clouddensity = 0.0;
        

        vec3 c = traceResult.color;


        vec3 lr = rd;
        //can have as many lightsources as i want yaay! :d

        //float ph = 0.4*ao + calcShading(ro, rd, rp, n);
        float ph = .70*ao + phongDiffuseFactor(-lr, n) + phongSpecularFactor(-lr,n, rd, 60.0);

        color = ph * vec3(0.4);//triplanarTexture(rp/2.0, n, iChannel0);


        float fg = fog(clouddensity, 0.800);
        vec3 fc = FOGCOLOR;
        //color = mix(color, fc, fg);
        color = mix(color, FOGCOLOR, fog(traceResult.rayt, FOGDENSITY)); 


    } else {
        float clouddensity = 0.0;
        color = sky(ro, rd);
        float fg = fog(clouddensity, 0.800);
        vec3 fc = FOGCOLOR;

        color = mix(color, fc, fg);
        color = mix(color, FOGCOLOR, fog(MAXT, FOGDENSITY)); 
    }



    color = clamp(color, 0.0, 1.0);
    fragColor = vec4(color, 1.0); 
}