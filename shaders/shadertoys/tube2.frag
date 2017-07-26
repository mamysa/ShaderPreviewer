// Trying out some of this cool terrain stuff. A bit too slow sadly.
#version 450
#define PI 3.141592653589793 

const float MAXT = 70.0f;
const float WORMHOLE_EPS = 0.01;
const float STARS_EPS = 0.0001;

const float FOGHEIGHT = 0.01; // background fog height. 
const float FOGFADEHEIGHT = 0.30; // background fog fade height - fades background fog into actual sky.
const vec3  FOGCOLOR = vec3(0.839, 1, 0.980);
const float FOGDENSITY = 0.000; 

const vec3  SUNDIRECTION = normalize(vec3(-1.0, -0.6, -0.9)); //direction of the sunlight
const vec3  SUNCOLOR =vec3(1.0); // sun color? 
const vec3  SKYCOLOR = vec3(0.513, 0.882, 0.945);




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

// polynomial smooth min (k = 0.1);
float smin( float a, float b, float k ) {
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float opSubtract( float d1, float d2 ) {
    return max(-d1,d2);
}

float opAdd(float d1, float d2) {
    return min(d1, d2);
}

vec3 opRepeat(vec3 p, vec3 c) {
    return mod(p,c)-0.5*c;
}



float ducky(vec3 p, out int mattype) {
    mattype = 1;

    vec3 p0 = rotateX(0.5)*p;
    vec3 p1 = vec3(p.x, p.y - cos(p.x*2.0) * 1.5, p.z);
    vec3 p2 = vec3(p.x, p.y + cos(p.x*2.0) * 0.75, p.z);
    vec3 p3 = rotateX(9.0)*vec3(p.x, p.y - cos(p.x*2.0)*1.0, p.z);

    float head = sdSpheroid(p-vec3(0.0, 1.0, -0.46), vec3(0.6));
    float leye = sdSpheroid(p0-vec3(+0.22,0.65,-1.4),vec3(0.12,0.16,0.12));
    float reye = sdSpheroid(p0-vec3(-0.22,0.65,-1.4),vec3(0.12,0.16,0.12));

    float liris = sdSpheroid(p0-vec3(+0.22,0.62,-1.51),vec3(0.04,0.05,0.02));
    float riris = sdSpheroid(p0-vec3(-0.22,0.62,-1.51),vec3(0.04,0.05,0.02));
    leye = opAdd(liris, leye);
    reye = opAdd(riris, reye);


    head = opAdd(head, leye);
    head = opAdd(head, reye);
    if (head == leye || head == reye) mattype = 3;
    if (head == liris || head == riris) mattype = 4;

    float ubeak = sdSpheroid(p1-vec3(0.0,-0.56,-1.0), vec3(0.26, 0.15, 0.4));
    float lbeak = sdSpheroid(p2-vec3(0.0,1.4,-0.9), vec3(0.26, 0.1, 0.4));
    float beak = min(ubeak*0.5, lbeak*0.5);

    float body = sdSpheroid(p-vec3(0.0,0.0,0.25), vec3(1.0, 0.7, 1.25));
    float tail = sdSpheroid(p3-vec3(0.0,1.1,-0.9), vec3(0.40, 0.25, 0.65));
    float flatbottom = sdPlane(p, vec4(0.0,1.0,0.0,-0.52));

    float f, h;
    h = smin(head, beak, 0.01);
    f = smin(tail*0.05, body, 0.13);
    f = smin(h,f,0.05);
    f = opSubtract(flatbottom, f);

    if (abs(f - beak) < 0.001) mattype = 2;
    return f;
}

float oceanSurface(vec2 st, out int mattype) {
    float time = iGlobalTime;
    float h = -0.2;
    h += 0.15*sin(dot(vec2( 0.0, 1.0),st)+time*0.10); 
    h += 0.06*sin(dot(vec2(-1.4, 0.8),st)+time*0.05); 
    mattype = 6;
    return h;
}



#if 0
float map(vec3 p, out int mattype) {
    #if 0
        float b1 = sdBox(q-vec3(0.0,1.0,0.0), vec3(0.5, 8.0, 1.0));
    mattype = 5;
    return b1 * 0.01;
    #endif
    #if 0
    p -=vec3(9.0,0.0,0.0);
    float c = cos(0.2*p.y);
    float s = sin(0.2*p.y);
    mat2  m = mat2(c,-s,s,c);
    vec3  q = vec3(m*p.xy,p.z);
    float b2 = udRoundBox(q, vec3(-0.2, 3.0, 1.0), 0.6);
    mattype = 5;
    return b2 * 0.01;
    #endif

    mattype = 5;

    float a = 9000.0;
    for (int i = 0; i < 8; i++) {
        float d = sdBox(p-vec3(-1.0,float(i)*1.5,float(i) * 2.0), vec3(1.0, 0.2, 1.0));
        a = opAdd(d, a);
    }

    return a;












    int ducky1_mat;
    int ducky2_mat;
    int ducky3_mat;

    float PI3h = 2.0*PI/3.0;
    vec3 p1 = rotateY(iGlobalTime*0.03)*p;
    p1 -= vec3(8.0, 0.0, 0.0);

    vec3 p2 = rotateY(iGlobalTime*0.03 + 1.0*PI3h)*p;
    p2 -= vec3(8.0, 0.0, 0.0);

    vec3 p3 = rotateY(iGlobalTime*0.03 + 2.0*PI3h)*p;
    p3 -= vec3(8.0, 0.0, 0.0);


    float d1 = ducky(p1, ducky1_mat);
    float d2 = ducky(p2, ducky2_mat);
    float d3 = ducky(p3, ducky3_mat);

    
    int unused;
    float f;
    f = opAdd(d1, d2);
    f = opAdd(f, d3);
    if (f == d1) mattype = ducky1_mat;
    if (f == d2) mattype = ducky2_mat;
    if (f == d3) mattype = ducky3_mat;
    return f;
}
#endif

#if 0
float map(vec3 p, out int mattype) {
    float f1 = 9000.0;
    float rep = 10.0;
    p = rotateZ(iGlobalTime*0.01)*p;
    mattype =2;
    for (float r = 0.0; r <= 2.0*PI; r+=PI*0.25) {
        vec3 p1 = rotateZ(r)*p; 
        p1 = p1-vec3(8.0,0.0,2.0);
        p1 = opRepeat(p1, vec3(0.0, 0.0, rep));
        p1 = rotateY(iGlobalTime*0.1+r+0.0)*p1;
        float b = udRoundBox(p1, vec3(0.5, 3.5, 1.0), 0.1);
        f1 = smin(f1, b, 0.1);
    }

    for (float r = 0.0; r <= 2.0*PI; r+=PI*0.25) {
        vec3 p1 = rotateZ(r)*p; 
        p1 = p1-vec3(8.0,0.0,5.0);
        p1 = opRepeat(p1, vec3(0.0, 0.0, rep));
        p1 = rotateY(iGlobalTime*0.1+r+0.10)*p1;
        float b = udRoundBox(p1, vec3(0.5, 3.5, 1.0), 0.1);
        f1 = smin(f1, b, 0.1);
    }

    for (float r = 0.0; r <= 2.0*PI; r+=PI*0.25) {
        vec3 p1 = rotateZ(r)*p; 
        p1 = p1-vec3(8.0,0.0,8.0);
        p1 = opRepeat(p1, vec3(0.0, 0.0, rep));
        p1 = rotateY(iGlobalTime*0.1+r+0.30)*p1;
        float b = udRoundBox(p1, vec3(0.5, 3.5, 1.0), 0.1);
        f1 = smin(f1, b, 0.1);
    }




return f1;
}
#endif

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
        dist = smin(dist, d, 0.05);
    }
    return -dist;
}

float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 0.7;

    const float octaves = 5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p*frequency);
        p *= 2.;
        amplitude *= 0.3;
        frequency *= 1.4;
    }
    return value;
}

vec4 map(vec3 p, out int mattype) {
    // tentacle
    vec3 bp = p;
    p.x -= noise(p.zz*0.2+iGlobalTime*0.02) * 2.25;
    p.y -= noise(p.xy*0.2+iGlobalTime*0.08) * 1.20;
    float tube = length(p.xy)-1.75; 

    for (int i = 1; i < 7; i++) {
        vec3 pt = p - vec3(0.0,0.0,float(i)*20.0);
        pt = rotateZ(1.75*float(i))*pt;
        pt = opRepeat(pt, vec3(0.0, 0.0, 60.0));

        float sphere = sdSpheroid(pt-vec3(0.0, 12.0, 0.0), vec3(6.0, 5.0, 6.0));
        float stube = length(pt.xz) - 1.2;
        float plane = sdPlane(pt, vec4(0.0,-1.0,0.0,0.0));

        stube = max(stube, plane);
        stube = smin(sphere, stube, 5.5);
        tube = smin(stube, tube, 0.75);
    }

    // walls
    float box1 = length(p.xy) -12.0;
    float box2 = length(p.xy) -10.0;
    box1 = opSubtract(box2, box1);
    tube = smin(box1, tube, 4.0);

    vec3 color = vec3(0.964, 0.482, 0.454); // default body color;
    vec3 cavity_color = vec3(0.403, 0.090, 0.074); // color between cells.
    vec3 veins_color = vec3(0.027, 0.117, 0.333);
    //vec3 veins_color = vec3(0.0, 1.0, 0.0);


    float f = worley_smooth(p*0.5-iGlobalTime*0.02)*0.7;
    color = mix(color, cavity_color, abs(f)*2.5);  

    // veins @OPTIMIZE this is slow!!! too many iterations for fbm! 
    float a = 0.23*(1.0-abs(fbm(p+iGlobalTime*0.05))); 
    float b = 0.11*(1.0-abs(fbm(p*2.0+iGlobalTime*0.07))); 
    float c = 0.02*(1.0-abs(fbm(p*9.2+iGlobalTime*0.01))); 
    float d = 0.01*(1.0-abs(fbm(p*15.2+iGlobalTime*0.01))); 
    float veins = a+b+c+d;
    float sm = smoothstep(0.0, 1.0, veins*1.6);
    color = mix(color, veins_color, sm);


    


    return vec4((tube-f-veins)*0.75, color);
}





float map(vec3 p) { int unused; return map(p, unused).x; }
float oceanSurface(vec2 p) { int unused; return oceanSurface(p, unused); }

struct TraceResult trace(vec3 ro, vec3 rd) {
    int mattype = 0;
    int watermat = 0;
    struct TraceResult traceResult = TraceResult(false, 0.0, vec3(0.0));
    float t = 0.02;
    float tmax = MAXT; 
    for (;t < tmax;) {
        vec3 rp = ro + rd * t;
        vec4 tr = map(rp, mattype);
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
        float rt = 0.2*float(t);
    	accum += w*(rt-map(p+rt*n));   
        w *= 0.8;
    }
    
    return clamp(1.0-accum, 0.0, 1.0);
}


vec3 calcOceanSurfaceNormal(vec3 rp) {
    vec2 eps = vec2(0.001, 0.0);
    vec3 normal = normalize(vec3( 
        oceanSurface(rp.xz - eps.xy) - oceanSurface(rp.xz + eps.xy),
        2.0 * eps.x, 
        oceanSurface(rp.xz - eps.yx) - oceanSurface(rp.xz + eps.yx) 
    ));

    return normal;
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

void getMaterial(in vec3 p, in int mattype, out vec3 matcolor) {
    matcolor = vec3(0.0);
    vec3 ducky_bodycolor = vec3(0.956, 1, 0.180);
    vec3 ducky_beakcolor = vec3(1, 0.321, 0.180);
    vec3 ducky_eyescolor = vec3(0.890, 0.890, 0.890);
    vec3 ducky_iriscolor = vec3(0.188, 0.188, 0.188);
    vec3 checkerboard1 = vec3(0.917, 0.698, 0.564);
    vec3 checkerboard2 = vec3(0.945, 0.949, 0.890);

vec3  OCEAN_COLOR = vec3(0.164, 0.345, 0.454); // water color


    if (mattype == 1) { matcolor = ducky_bodycolor; }
    if (mattype == 2) { matcolor = ducky_beakcolor; }
    if (mattype == 3) { matcolor = ducky_eyescolor; }
    if (mattype == 4) { matcolor = ducky_iriscolor; }
    if (mattype == 5) { 
        // have to offset positition otherwise there are some texture 
        // sampling errors. Need to look into fixing in.
        p = floor(p*1.00000); 
        if (mod(p.x+p.y, 2.0) == 0.0 
        
        ) {
            matcolor = checkerboard1; 
        } else {
            matcolor = checkerboard2;
        }
    }  // ground plane

    if (mattype == 6) {
        matcolor = OCEAN_COLOR ;
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


    vec3 color;// = FOGCOLOR;





    


    vec3 rd = normalize(vec3(st, 1.0));
    //rd=rotateY(-1.55)*rd;
    rd = rotateY(-mouse.x) * rotateX(mouse.y)  * rd;
    rd = normalize(rd);

    //vec3 ro = vec3(0.25, 0.0, 0.0); 
    vec3 ro = vec3(3.0, 0.0, -7.0); 
    ro += up * 0.22 * normalize(vec3(0.0, 1.0, 0.0));
    ro += forward * 0.5*  normalize(vec3(0.0, 0.0, 1.0));
    //ro += iGlobalTime*0.50*normalize(vec3(0.0, 0.0, 1.0));


    TraceResult traceResult = trace(ro, rd);
    if (traceResult.hit) {

        vec3 rp = ro+traceResult.rayt*rd;

        vec3 n = calcNormal(rp);

        // calculate surface thickness 
        float thickness = calcSurfaceThickness(rp-n*0.01, rd, 20.0) * 4.0;
        float ao = calcAO(rp, n);

        

        vec3 c = traceResult.color;
        vec3 ld = normalize(vec3(1.0, -1.0, 1.0));

        float ph = 0.5*ao + phongDiffuseFactor(-SUNDIRECTION, n) + phongSpecularFactor(-SUNDIRECTION,n, rd, 200.0);
        float ph2 = phongDiffuseFactor(-SUNDIRECTION, n) ;
        color = ph*c+c*thickness;

    } else {
        color = sky(ro, rd);
    }



    color = clamp(color, 0.0, 1.0);
    fragColor = vec4(color, 1.0); 

}