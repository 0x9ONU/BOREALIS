This is where documentation for this project would go, but this is an explaination instead

# Project Rundown
Each project has it's own main file that runs on the MCU. This project folder is the template you can use to create a new project for you to use and make your own code for testing. By default, it includes a simple example in main.cpp, a baseline Makefile, and a README (this file).

The main.cpp file is the main program that will run on the MCU. Other files like libraries, functions, and classes will be needed, and in order for the computer to understand that it needs these files, we must declare their useage in the main.cpp file using include statements. 

Additionally, the computer needs to know where to find these extra files within this project, especially if the extra files are not in the project folder. An example of this is the libDaisy library we are using. This file is found at the BOREALIS/DaisyExamples/libDaisy path. For the computer to know where this file is, it should be included in the Makefile somewhere. It seems like it just works for now, but if changes are made then personal research will be needed.

## IMPORTANT
To be able to run build, program, and do other tasks on a new project, you need to add the name of the overall project folder to the "tasks.json" file found in the ".vscode" folder. There is a block of text that explains this process in that file.



