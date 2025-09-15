
vec3[] d4 = vec3[]  (
vec3(1.,1.,1.),
vec3(1.,-1.,-1.),
vec3(-1.,-1,1.),
vec3(-1.,1,-1.)
);

vec3 ifs_color, ro, color, id;
float ifs_scale = 1.7;
mat2 rot(float a) {return mat2(cos(a),-sin(a),sin(a),cos(a));}

vec2 hexagon( in vec2 p, in float r )
{
    const vec3 k = vec3(-0.866,0.5,0.577);
    p = abs(p);
    p -= 2.0*min(dot(k.xy,p),0.0)*k.xy;
    p -= vec2(clamp(p.x, -k.z*r, k.z*r), r);
    return p; //length(p)*sign(p.y); //return the transformed p instead of distance
}

float hexDist( in vec2 p, in float r )
{
    const vec3 k = vec3(-0.866,0.5,0.577);
    p = abs(p);
    p -= 2.0*min(dot(k.xy,p),0.0)*k.xy;
    p -= vec2(clamp(p.x, -k.z*r, k.z*r), r);
    return length(p)*sign(p.y);
}

float cube(vec3 p) {

    vec3 d = abs(p) - .5;
    return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float de(vec3 z, float iters) {

    vec3 zz = abs(ro-z);
    float shell = max(max(zz.x,zz.y),zz.z) - .01;
    
    float dd = 1.7;
    id = round(z/dd);
    z.xz -= dd*id.xz;  
    

    float floor = z.y+.5;
    
    vec3 min_vtx;
    float min_dist,dist_to_vtx;

    ifs_color = vec3(0.);

    vec2 zzz = z.xz; //z.xz*rot(length(id.xz));
    
    float column = hexDist(zzz,.10); //add vertical hex columns

    z.xy = hexagon(z.xy,.15);
    z.xz = hexagon(z.xz,.15);

    for (float i=0.; i<6.; i++) {
       
        min_vtx = d4[0];
        min_dist=length(z-d4[0]);
        for (int j=1; j<4; j++) {
        
            dist_to_vtx=length(z-d4[j]); 
            if (dist_to_vtx<min_dist) {min_vtx=d4[j]; min_dist=dist_to_vtx;}
            
        }
        
        z = min_vtx + ifs_scale*(z-min_vtx);

#define S(p) step(0.,p.x*p.y)
        ifs_color += vec3(S(z.xy),S(z.yz),S(z.zx));

    }

    ifs_color /= iters ;

    float dz = pow(ifs_scale, iters );

    return max( min(floor,min(cube(z)/dz, column)),-shell);
}



vec3 abnormal(vec3 p) {
    vec2 dx=vec2(1,-1);
    vec2 dp=.001*dx;
    float iters = 3.; //we don't  need as many iterations here
    return normalize ( 
          dx.xyy*de(p+dp.xyy,iters) 
        + dx.yyx*de(p+dp.yyx,iters) 
        + dx.yxy*de(p+dp.yxy,iters)
        + dx.xxx*de(p+dp.xxx,iters)
        );  
}



void march(vec3 ro, vec3 rd) {

    float t = 0., numRF=0.;

    for(float i = 0.; i < 60.; i++) {
    
        vec3 p  = ro + rd*t;
        float d = de(p, 8.);

        ifs_color = cos(ifs_color*1.2*(1.+abs(sin(iTime))));

        ifs_color *= ifs_color;

        if ( i > 7.)
        color += .1*ifs_color*exp(-t*t/2.)  ;
          
        vec3 ldir=normalize(vec3(1,1,0));
        
        if (d< 1e-5) {
        
           if (numRF > 3.) break;
           vec3 nn=abnormal(p);
           rd = reflect(rd,nn);
           color *= (1.+.3*sqrt(numRF));
           t  -= .02; 
           numRF ++;  
   
        }
        
        t += d;
         
    }
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p  = (2.*fragCoord.xy-iResolution.xy)/iResolution.y;
    
#define T mod(iTime,300.)*.2

    ro = vec3( .6+.3*sin(T),1.6,T);
    
    vec3 rd = normalize( vec3(p, 1.4) ) ; 

    rd.xz *= rot(.4*sin(T));

    rd.yz *= rot(.5);
    march(ro, rd);
  
    
    fragColor = vec4( color*color, 0);
      
}
