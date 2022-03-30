

## Rationale

We have moved to 3.1 deferred rendering pipeline. Historically decals have been rendered in the targeted objects fragment-shader. 
For that reason it makes sense to limit decals to a single few textures so that they can all be rendered in one call. To achieve this
while still allowing for many textures we propose generating a texture atlas with all used decals in runtime.

## Overview

The bullet points are relevant to be aware of

 - Automatic scaling of decals if texture-size limit is reached. (halfing)
 - Decals dimensions will be limited to a power-of-2