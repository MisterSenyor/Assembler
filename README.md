# Assembler

16-bit assembler built as part of a university project, complete with macro layouts.
# Usage

In order to use the assembler, you must download it locally and compile the program using the makefile (with the `make` command).

Once compiled, the `assembler` executable recieves file names as command-line arguments. The arguments should be provided such that the argument `<filename>` corresponds to the file `<filename>.as`.

The files are conveniently documented and segmented such that modifying them is made quite convenient.
# Output

The first output file will always be `<filename>.am`. This is the same script, but after having gone through the pre-processing stage.

The next output files will not be created if the program fails:
- `<filename>.ent` - the entry points declared in the file and their addresses.
- `<filename>.ext` - the external variables declared in the file and the addresses of the lines where they are used.
- `<filename>.ob` - the output file of the compiled code. The format is such that nibbles A-E represent a full 20-bit word (in a theoretical 20-bit system).
# Examples
Examples of the program's usage are also provided, in the `Examples` folder. The examples detail all possible errors that the assembler will locate and warn about. The error messages recieved from the unsuccessful runs are also located int he `Examples` folder, labeled `Examples/bad_runs.png`.

Other examples demonstrate successful runs, complete wiwth
