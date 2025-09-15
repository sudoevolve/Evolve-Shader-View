void mainImage(out vec4 O, in vec2 u)
{
    O = vec4(0);

    for( int i=0; i<5; i++ )
    for( int j=0; j<5; j++ )
    {
        vec2 r = vec2(i,j) - 2.;
        O += exp2(-7.*dot(r,r))*texture(iChannel0, (u+r)/iResolution.xy );
    }

    O = .5*sqrt(O);

    O.rgb = pow(O.rgb, vec3(1.0/2.2)); // 👈 手动 gamma 校正
}

