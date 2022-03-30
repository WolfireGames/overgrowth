---
format: Markdown
...

# Introduction 

The overgrowth AI needs to be mobile. The reason for this is the players inherent mobility coupled with the AI's expected ability to match the player in most situations.

The current AI which walks along a simple navmesh and occasionally jumps when there are no other options is therefore not sufficient for the final product.

To improve the mobility of the AI, two modifications are suggested to the current pathfinding algorithms

1. The AI continously to take shortcuts on the navmesh by directly jumping to a future point. This can simply be done by searching from the last to the first point in a currently calculated path and have the AI predict if a jump should be possible via sweeping a collision mesh along an expected jump path. If the sweep succeeds the AI attempts to jump and then promptly recalculates it's pathfinding.

2. Jump point hints. Recast supports so called "off mesh connections". These are essentially two manually placed points on the navmesh that is included as a possible path when searching for a path in the navmesh. This allows the AI to be aware of a possible jump before hand, and elect to take such a route if it's deemed shorter. This is suggested for places that function as "dead ends" in other situations as there would be little reason for the AI to go into the alley and look for a predicted jump point.

