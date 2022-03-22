# Overgrowth Open Source
This is the official repository for the source code of the game [Overgrowth]. Only the code is available here; the game data (such as art assets and levels) can only be legally obtained by purchasing a copy of Overgrowth from [Wolfire Games].

The intent of this repository is to allow people to‥

- Run the open source code along with the commercial data in order to perform experiments for educational purposes.
- Propose changes that can be merged back into the commercial game.
- Create modifications for Overgrowth that would otherwise be impossible.
- Use useful code snippets in their own projects.
- Create their own commercial "total conversions" that use a completely new set of assets.

If you would like to distribute any of the original Overgrowth assets, you must obtain explicit written permission from Wolfire Games.

## Compiling
Information on how to compile and run the commercial Overgrowth game using the open source code can be found in [COMPILING.md].

## Contributing
This repository is entirely community-owned. That means you are welcome to submit your own improvements, and us volunteer maintainers will review, give feedback, and consider your code for inclusion when we have time.

For more detailed information on this process, read [CONTRIBUTING.md].

## License
The code in this repository is licensed under `Apache-2.0`.

`Apache-2.0` is what's called a "permissive" open source license, meaning you are allowed to use it for more or less whatever you want, including in closed source projects. Before contributing or using the code in this repository for anything other than personal use, read the full license in [LICENSE.md].

Licenses are hard to understand, and the `Apache-2.0` license is no exception. Reading [tl;drLegal's summary of `Apache-2.0`] will give you an overview, and below is a summary in plain English of what the license means in practice. This is just an interpretation of what we think are the most important parts of the license, if you are going to use the license, make sure to read and understand the actual license.

If you distribute a compiled program using the code, or the code itself, you must do the following:

### Include the full contents of the `LICENSE` file

This can be somewhere in the program itself, or in a separate file. The purpose is to make it clear what license you're using the code under. For example, you can have a file called `LICENSES`, and in it you can have a line saying `This software uses code from Overgrowth under the Apache-2.0 license, read the full license in the file named LICENSE_APACHE.`.

### Mark files you've changed with a notice

If you distribute the code itself, write in each file you modify that you have modified it. For instance, the files have some text at the top which is called a "boilerplate notice", you can put a notice that you have modified it there, so it looks someting like this:

```
// This file is part of Overgrowth
// Copyright 2022 Wolfire Games
//
// This file has been modified by John Doe.
//
// <the license boilerplate comes here>
```

### Do not remove any copyright noticies or similar

If you see something like `Copyright 2022 Wolfire Games` or similar in the code or elsewhere, you're not allowed to remove that.

[Overgrowth]: https://overgrowth.wolfire.com
[Wolfire Games]: https://wolfire.com
[COMPILING.md]: COMPILING.md
[CONTRIBUTING.md]: CONTRIBUTING.md
[LICENSE.md]: LICENSE.md    
[tl;drLegal's summary of `Apache-2.0`]: https://tldrlegal.com/license/apache-license-2.0-%28apache-2.0%29
