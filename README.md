# Overgrowth Open Source
This is the official repository for [Overgrowth]'s source code. Only the code is available here; the game data (such as art assets and levels) can only be legally obtained by purchasing Overgrowth from [Wolfire Games].

This repository intends to allow people to‥

- Run the open source code with the commercial data to perform experiments for educational purposes.
- Propose changes to be merged back into the commercial game.
- Create modifications for Overgrowth that would otherwise be impossible.
- Use helpful code snippets in their projects.
- Create their own commercial "total conversions" that use an entirely new set of assets.

If you would like to distribute any of the original Overgrowth assets, you must obtain explicit written permission from Wolfire Games.

## Compiling
[COMPILING.md] contains instructions on compiling and running the commercial Overgrowth game using the open source code.

## Contributing
This repository is entirely community-owned. That means you are welcome to submit improvements, and volunteer maintainers will review, give feedback, and consider your code for inclusion when they have time.

[CONTRIBUTING.md] has more information on this process.

## License
The code in this repository is licensed under `Apache-2.0`.

`Apache-2.0` is a "permissive" open source license, meaning you are allowed to use it for more or less whatever you want, including in closed source projects. [LICENSE.md] contains the full license. Please read and understand it before contributing or using the code for anything other than personal use.

Licenses are hard to understand, and the `Apache-2.0` license is no exception. Reading [tl;drLegal's summary of `Apache-2.0`] will give you an overview, and below is a summary in plain English of what the license means in practice.

If you distribute a compiled program using the code, or the code itself, you must do the following:

### Include the entire contents of the `LICENSE` file

The license can be somewhere in the program itself or a separate file. The purpose is to clarify under what license you're using the code. For example, you can have a file called `LICENSES`, and there you can have a line saying `This software uses code from Overgrowth under the Apache-2.0 license, read the full license in the file named LICENSE_APACHE.`.

### Mark files you've changed with a notice

If you distribute the code itself, in each file you modify, write that you have modified it. For instance, the files have some text at the top called a "boilerplate notice", you can put a notice that you have changed it there, so it looks something like this:

```
// This file is part of Overgrowth
// Copyright 2022 Wolfire Games
//
// John Doe modified this file.
//
// <the license boilerplate comes here>
```

### Do not remove any copyright notices or similar

You're not allowed to remove things like `Copyright 2022 Wolfire Games` or similar from the code or elsewhere.

[Overgrowth]: https://overgrowth.wolfire.com
[Wolfire Games]: https://wolfire.com
[COMPILING.md]: COMPILING.md
[CONTRIBUTING.md]: CONTRIBUTING.md
[LICENSE.md]: LICENSE.md    
[tl;drLegal's summary of `Apache-2.0`]: https://tldrlegal.com/license/apache-license-2.0-%28apache-2.0%29
