// Trying out some of this cool terrain stuff. A bit too slow sadly.
#version 450
#define PI 3.141592653589793 

const float WORLEYMAXDIST = 8.0; // worley noise max distance
const float MAXLENGTH = 190.0; // maximum ray length
const vec3 SUNDIRECTION = normalize(vec3(1.0, -0.3, -0.4)); //direction of the sunlight
const vec3 SUNCOLOR = normalize(vec3(0.960, 0.619, 0.180)); // sun color? 
const float FOGHEIGHT = 0.01; // background fog height. 
const float FOGFADEHEIGHT = 0.2; // background fog fade height - fades background fog into actual sky.
const vec3 FOGCOLOR = vec3(0.996, 0.878, 0.603);


in vec2 fragTexCoord;
out vec4 fragColor;
uniform vec2 resolution;
uniform vec2 mouse;
uniform float forward;
uniform float up;

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
    
    float dist = 1.0/0.0;
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++) {
        vec2 currentTile = tileCoord + vec2(x, y);
        vec2 point = currentTile + random(currentTile);
        // not entirely correct but makes fakes trees look better... imho
        dist = min(dist, sqrt(length(point-uv))); 
    }
    dist = clamp(dist, 0.0, WORLEYMAXDIST) / WORLEYMAXDIST;
    return dist;
}

float fbm(vec2 st) {
    float value = 0.0;
    float amplitud = 0.5;
    float frequency = 0.2;

    const float octaves = 3;
    for (int i = 0; i < octaves; i++) {
        value += amplitud * noise2D(st * frequency);
        st *= 2.;
        amplitud *= .5;
        frequency *= 1.20;;
    }
    return value;
}

float trees(vec2 st) {
    float a = fbm(st.xy * 1.9) + 1.0;
    float x = 2.0 * worley(st * 7.5) - 1.0;
    return a+(min(-x, 0.91)  * 2.5 * fbm(st * 20.0)); 
}

float terrain(vec2 st) {
    float a = fbm(st.xy * 1.9) + 1.0;
    a = abs(1.0-a)  * 19.0 - 4.0;
    float b = fbm(st.yx * 41.3);
    return a - b*(a*0.20); 
}

float map(vec2 st, out int mattype) {
    float terrain = terrain(st) + 2.0; 
    float trees = trees(st * 1.1) * 1.4 + 0.35; 
    float h = max(trees, terrain);
    if (h == terrain)  mattype = 1; 
    if (h == trees)    mattype = 2; 
    return h;
}


vec2 trace(vec3 ro, vec3 rd, out int mattype) {

    float height = -1.0;
    float t = 0.02;
    float tmax = MAXLENGTH;

    for (;t < tmax;) {
        int m = 0;
        vec3 rp = ro + rd * t;
        float h = map(rp.xz, m);
        float d = rp.y - h;

        if (d < 0.01) {
            height = h;
            mattype = m;
            break;
        }
        t += 0.5 * d;
    }

    return vec2(height, t);
}

vec3 getNormal(vec3 rp) {
    int unused = 0;
    vec2 eps = vec2(0.01, 0.0);
    vec3 normal = normalize(vec3( 
        map(rp.xz - eps.xy, unused).x - map(rp.xz + eps.xy, unused).x,
        2.0 * eps.x, 
        map(rp.xz - eps.yx, unused).x - map(rp.xz + eps.yx, unused).x 
    ));
    return normal;
}


vec3 getShading(vec3 p, vec3 ld, vec3 n) {
    int unused;
    // lambert
    float kd = max(0.0, dot(-ld, n));
    // cast shadow ray...

    vec3 a = p + vec3(0.0, 0.1, 0.0);
    vec2 s = trace(a, -SUNDIRECTION, unused);
    float sh = (s.x == -1.0) ? 1.0 : 0.0;

    return (kd * SUNCOLOR * sh) ;
}
// that is really shitty AO but at least unlit fragments do not look so plain... :)
float bad_ao(vec3 n) {
    return abs(dot(n, vec3(0.0, 1.0, 0.0))); 
}

float fog(float dist) {
    const float density = 0.008;
    return  1.0 - 1.0/exp(pow(dist * density, 2.0));
}

// borrowed this from a certain thread on pouet
vec3 postprocess(vec3 color) {
    const float contrast   = 0.15;
    const float brightness = 2.4;
    const float gamma = 1.5;  // higher => darker; lower => brighter
    const float saturation = 1.4;

    color = color * brightness;
    color = pow(color, vec3(gamma));
    color = color * 0.5 + contrast * 0.5;

    float luminance = dot(vec3(0.2126, 0.7152, 0.0722), color);
    color = luminance + (color - luminance) * saturation;

    return clamp(color, 0.0, 1.0);
}

vec3 sky(vec3 ro, vec3 rd, vec2 st) {
    vec3 color = vec3(0.8, 0.5, 0.4);
    // borrowed it from one of iq's demos, hopefully no one will notice...
    color += smoothstep(0.3, 0.6, fbm(rd.xz * 90.0 / rd.y));

    float d = dot(-SUNDIRECTION, rd); // sun??
    if (d > 0.0)          
        color = mix(color, vec3(1.0, 1.0, 0.8), pow(d, 20.0));
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
    rd = rotateY(-mouse.x+0.785398) * rotateX(mouse.y)  * rd;
    rd = normalize(rd);
 
    vec3 ro = vec3(0.0, 12.5, 0.0); 
    ro += up * normalize(vec3(0.0, 1.0, 0.0));
    ro += forward * normalize(vec3(-1.0, 0.0, 1.0));;

    int mattype = 0;
    vec2 height = trace(ro, rd, mattype);

    vec3 rp = ro + height.y * rd;
    vec3 n = getNormal(rp);
    vec3 kd = getShading(rp, SUNDIRECTION, n);

    // texture slope fade constants
    // anything below smin slope uses grass texture, anything above uses rock texture,
    // values between are lerped.
    float smin = 0.05;  
    float smax = 0.15;

    // terrain detail texture
    float vmin = 0.4;
    float vmax = 1.0;

    vec3 color1 = vec3(1.0, 0.0, 0.0);
    vec3 color2 = vec3(1.0, 0.0, 0.0);
    if (mattype == 1) {
        color1 = vec3(0.925, 0.964, 0.572); //  grass color;
        color2 = vec3(0.474, 0.368, 0.207); // rock 
    }
    if (mattype == 2) {
        color1 = vec3(0.611, 0.686, 0.113); //  tree base;
        color2 = vec3(0.760, 0.913, 0.427); //   tree top;
        color1 = color1 + vec3(noise2D(rp.xz * 9.0),  noise2D(rp.zx * 2.0), noise2D(rp.zx)) * 0.20;
        color2 = color2 + vec3(noise2D(rp.xz * 5.0),  noise2D(rp.zx * 9.0), noise2D(rp.zx)) * 0.20;
        smin = 0.09;
        smax = 0.24;
        vmin = 0.5;
        vmax = 1.0;
    }

    float slopefactor =  1.0 - abs(dot(n, vec3(0.0, 1.0, 0.0)));
    float slopefactor2 = clamp(slopefactor, smin, smax);
    slopefactor2 = (slopefactor2-smin) / (smax - smin);
    
    vec3 color = mix(color1, color2, slopefactor2);
    float variation = (noise2D(rp.xz *225.0) + noise2D(rp.zx * 225.0)) * 0.5 ;
    variation = clamp(variation, vmin, vmax);
    color *= variation;
    float ao = bad_ao(n);
    color = color * 0.3 * ao + color * kd;

    const vec3 fogcolor = vec3(0.996, 0.878, 0.603);
    color = mix(color, fogcolor, fog(height.y));

    if (height.x == -1.0) { color = sky(ro, rd, st); }
    color = postprocess(color);
    fragColor = vec4(color, 1.0); 
}




// I like this one!
    #if 0
    float a = fbm(st.xy * 1.4);
    a += fbm( (st.yx + vec2(100.9, 200.0)) * 1.2);
    a = abs(1.0-a) + noise2D(st * 0.3) * 0.5;


    a = a * 32.0;
    float b = fbm(st.yx * 31.3);
    a = a - b*(clamp((a-4.0), 0.0, 10.0)*0.20); 
    return a ;
    #endif


    //float a = abs((sin(st.y * 2.0 ) + cos(st.x * 2.0)));

    //cool minecraft like trhing
    //float a = mod(abs(floor(st.y)) + abs(floor(st.x)), 35.0);
    //return min(a, 22.0);

    
// canyon like thing
#if 0
    float a = floor(5.0 * noise2D(st * 1.9)) * 1.6;
    float b = fbm(st * vec2(4.2, 4.0) * 8.5);
    b = abs(1.0-b) * 1.1;
    float c = fbm(st * 8.3) * 1.35;
    return max(max(a,b)+c, 2.0);
#endif
