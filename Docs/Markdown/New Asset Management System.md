#New Asset Management

#Motivation
For reasons X we need a new asset loading system which can handle asynchronous
loads away from the main thread. 

#Design

The asset loading system functions in multiple steps.

- A Requst for an asset it made
- It's checked weather or not the asset is already loaded.
- If the asset isn't loaded, it's loaded.
- The data for the asset is then read from disk in a dedicated thread
- The asset is then converted if necessary to the internal format in another
  thread.
- Then the loading is synced into the main thread were any necessary memory
  moves are performed (like into OpenGL)


In the old systems there are separate AssetInfo structures, it's not yet
determined what their use is and if they should be merged into the main path of
normal assets. It appears it's mainly a structure used for loading XML files
containing structural info, which could also just be considered an Asset in
itself from the perspective of the Asset Loader.

The system should also support the ability to reload assets.
