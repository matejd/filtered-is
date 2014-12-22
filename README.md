Filtered Importance Sampling in WebGL
=====================================

This was coded as a part of my diploma thesis on physically-based shading (or rendering).
Importance sampling (IS) is essentially a Monte Carlo integration method used in rendering
diffuse and glossy surface reflections (BRDFs evaluated in semi-random directions across a hemisphere domain).
But plain importance sampling requires a large number of samples to produce accurate
results in many cases. Filtered IS adds filtering (doh) of environment maps (mipmaps)
to reduce the number of needed samples.
See resources for more theory. Online [here](http://matejd.github.io/filtered-is/).

![Screenshot](http://matejd.github.io/filtered-is/walt-importance-sampling.png)


Features
--------

- Importance sampling Trowbridge-Reitz normal distribution fuction (see e.g. http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html)
- Halton quasi-random sequences
- HDR (RGBM encoded) environment map in panoramic format


Resources
---------

[SIGGRAPH 2010 Course: IS for Production Rendering](https://sites.google.com/site/isrendering/)

[Importance Sampling for Production Rendering](http://www.igorsklyar.com/system/documents/papers/4/fiscourse.comp.pdf)

[GPU-Based Importance Sampling](http://http.developer.nvidia.com/GPUGems3/gpugems3_ch20.html)

[Physically Based Shading SIGGRAPH 2014](http://blog.selfshadow.com/publications/s2014-shading-course/)

[Physically Based Shading SIGGRAPH 2013](http://blog.selfshadow.com/publications/s2013-shading-course/)
