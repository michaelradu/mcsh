# Minimal (Hackable) C Shell
Just as the name suggests, it is a minimal shell implementation in C inspired by lsh but with added functionalities. Being still in early development, it demonstrates the basics of how a shell works: read, parse, fork, exec and wait, but still suffers from the hurdles that lsh has:

* Commands must be on a single line.
* Arguments must be separated by whitespace.
* No quoting arguments or escaping whitespace.
* No piping or redirection.
* Only builtins at the moment are `cd`, `help`, `exit` and my addition of `history`.
* Config files are still in very early development and only support prompt changing at the moment

## Purpose
This project was born out of curiosity for how a shell works and the need to build and document something more complete than other tutorials. It is unlikely that it will replace zsh in my current workflow but building it up to the point of it being on par with zsh for my very basic use case (and probably others' too) is my current goal. Whether or not I will choose to keep on pursuing said goal, or whether I will keep maintaining the shell after a certain point is up to debate, as this is only a "for fun" project.

## License

This project is licensed under the [GPLv3 License](LICENSE).

## TO DO:

- Add quote and backslash escaping
- Add piping and redirection
- Add more standard builtins (pretty much already done just requires me to add them to the project)
- Improve config file support (aliases, colors, random prompts, custom file locations, etc)
- Improve history support
- Add command history scrolling and searching
- Add tab autocompletion
- Add eyecandy (づ｡◕‿‿◕｡)づ
- Rebase the source to make it more readable and less sleep deprived coding

## Contributing
Contributions to this project are welcome! If you find any issues or have suggestions for improvements, please open an issue or submit a pull request.
