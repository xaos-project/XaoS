 * Shut up c++ compiler warnings and errors
 * Added undo/redo mechanizm
 * Free _ALL_ memory before exit. This help to debug memory leaks. Also BeOS
   shared libraries prabably don't free it automagically.
 * Windows:
 	* cut&paste support
	* fixed some dX releated stuff
	* support for separators
	* plastic border around the fractal
 * BeOS:
 	* Fix some BeOS translator releated stuff
 	* Support for multiple file drops
	* Fix colormap functions in fullscreen driver
	* Use correct (BeOSish) way to exit
	* Don't exit on fatal errors in translator
	* Cleanup everything once translator is done
	* Don't let user to enter incorect image sizes in translator
	   configuration
 * Fixed replay algorithms
 * Small speedups in interruptible calculation mode
