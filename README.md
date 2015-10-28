UniDOS
==============

UniDOS is an emulator to run Microsoft DOS executables.

Built on top of [Unicorn engine](http://www.unicorn-engine.org), so UniDOS should be able to work wherever Unicorn works.


Compilation
-----------

[Unicorn](http://www.unicorn-engine.org) must be installed before going to the next step.

On Mac OSX/Linux/BSD, simply run *make* to compile **unidos**.

    $ make


Usage
-------

Simply pass the DOS file and options to *unidos*. Below is an example on how to run a sample tool *pkunzjr.com* with our emulator.

To run *pkunzjr.com*, do:

    $ ./unidos bin/pkunzjr.com

To unzip sample file *a.zip*, do:

    $ ./unidos bin/pkunzjr.com -o a.zip


Status
-------

Currently UniDOS can only handle COM file with some basic DOS interrupt services (INT 20h, INT 21h). Pull-requests to extend this tool are welcome.


License
-------

This tool is released under the [GPL license](COPYING).


Author
-------

Nguyen Anh Quynh (aquynh at gmail dot com)
