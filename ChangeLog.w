 * Cleanup code around catalog loading (Win95 port seems to crash there)
 * Process events before entering main loop to let driver report the correct
   size by resize event.
 * Reordered uninterruptible calculation to allow better guessing
 * Guess a lot more pixels
   (zooming is now visibly faster, hope still accurate enought)
 * Fixes in __fabsl code
 * Small fixes and improvements in tutorials
