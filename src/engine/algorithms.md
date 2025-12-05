## Algorithms

XaoS implements several novel algorithms which are described here.

The main idea behind XaoS is that it is not necessary to calculate the
whole image in every frame; most pixels were already calculated by the
previous frames. You usually don’t have exactly the pixels you want, but
all within a range lower than a step between pixels are acceptable. That
is why the image flickers a bit and why points do not blink randomly as
in recalculated animations.

This document describes some of the most important algorithms in XaoS:

  - Saving Previous Pixels
  - Approximation Algorithm
  - Moving Pixels to New Positions
  - Calculating New Pixels
  - Symmetry
  - Calculation of Mandelbrot Set
  - Dynamic Resolution
  - Autopilot


### Saving Previous Pixels

Ideally, all recalculated points should be saved and used for building
successive frames. I could not figure out a practical way to implement
this. To save all frames for half an hour would require 24 Mb of memory,
and searching the saved frames would be more computationally expensive
than recalculating an entirely new frame.

One way was later used by the program Frang. It remembers all pixels as
triplets of (x,y,value), and when it builds a new image, it draws all
the pixels that it remembers to that image and then browses the image
and fills it with new pixels. (Possibly an
<span class="small">RLE</span> encoding should be used for calculated
pixels to conserve memory.) Frang actually uses an algorithm that takes
away pixels from the screen, so it behaves in exactly the same way as
the algorithm described here. On the other hand, this method seems to
require much more memory than XaoS’ algorithm, and drawing
pixels/browsing the image costs quite a lot, so the algorithm described
here seems to be faster, since it never requires examining the whole
image, and the new image is constructed using block move operations.

For this reason, only the last generated frame is used as a reference.
This way the memory requirements are proportional to *xsize \* ysize*.
It can be shown that this method is only about 2–5% slower during
zooming. (Of course unzooming back to once browsed areas is much
slower.)

Because only the previous frame is used, another optimization can be
performed: The imaginary and real parts of the calculated image are not
precise, since they are the result of successive iterations of the
algorithm. In order to prevent errors from being propagated to the
following frames, their exact coordinates need to be known. Fortunately,
it isn’t necessary to save their values since it is known that all real
components in a row and all imaginary components in a column are equal.
Thus, the only things that must be saved are the real components for
every row and the imaginary components for every column.

This allows for a substantial speed-up in approximation because the
calculation requires less data. Of course, some rows and columns fall
out of the threshold and new ones need to be calculated to fill in the
gaps in the frame.

Obviously, much less work is done than in a brute-force calculation:
there are only *xsize + ysize* calculations instead of *xsize \* ysize*.
So the main loop in XaoS looks like this:

  - Make approximations for rows
  - Make approximations for columns
  - Move old pixels to their new positions
  - Calculate pixels for which there is no good approximation for their
    row
  - Calculate pixels for which there is no good approximation for their
    column, but there is one for their row


### Approximation Algorithm


#### Introduction to problem

You can see that the approximation algorithm is central to the
implementation of XaoS. If a guess is incorrect the image will look
strange, boundaries will not be smooth and the zoom will flicker. On the
other hand, if it adds more new rows or columns than required, zooming
will become much slower. Also, if doubling should happen (i.e., using an
old row or column more than once) the resolution will lower and the
image will look jagged. It is important to keep the increasing imaginary
and real components in the correct order. If a row and column of complex
coordinates follows one with higher coordinate values, an improved
approximation can be attained by swapping their values.

The algorithm needs to be relatively fast. It is only used for *xsize +
ysize* values, but if its speed is proportional to *O(n^2)*, it can be
slower than a whole recalculation of the image. Speeds of *O(n)* or *O(n
\* log(n))* are acceptable.


#### Some simple algorithms to solve it

Initially, a very simple algorithm was used:

Find the old row/column nearest the row/column that needs to be
regenerated. If the difference between them is less than one step (*step
= (end - beginning) / resolution*) then use it. Otherwise, recalculate a
new one.

Finding the nearest row/column pair is very simple since it is always
greater than or equal to the pair needing to be generated.

Surprisingly, this simple algorithm has almost all the problems
described above. Doubling was fixed by lowering the limit to *step / 2.*
This caused a considerable slowdown so the limit was returned to *step*.
Instead, the algorithm was changed to search for only row/column pairs
that are greater than the previous frame’s row/column pairs. This is the
algorithm that was used in version 1.0.

This algorithm still added too many new rows and columns, and did not
generate smooth boundaries. For version 1.1 a heuristic was added that
preferred approximating rows/columns with lower values. This way it did
not occupy possible rows/columns for the next approximation. The result
was a speedup by a magnitude of four. In versions 1.1 to 2.0 many
improvements were made to the heuristic to give it added performance.
The following example tries to explain how complicated the problem is (O
is the old coordinates and X is the values to be approximated):

``` example
        X1        X2        X3        X4        X5        X6        X7
O1 O2                    O3 O4 O5                   O6 O7 O8
```

The normal algorithm will aproximate X1 by O2, X3 by O4 but nothing
more. For the algorithm with threshold step instead of *step / 2*:

``` example
  O2 to X1
  O3 to X2
  O4 to X3
  O5 to X4
  O6 to X5
  O8 to X6
```

But this will fail with X7. The second algorithm which relies on lower
values will do the following:

``` example
  O1 to X1
  O3 to X2
  O4 to X3
  O5 to X4
  O6 to X5
  O7 to X6
  O8 to X7
```

O1 to X1 is wrong. And there is many and many other situations that may
occur. But you may see that the normal algorithm will calculate 4 new
rows/columns but the heuristic saves all of these calculations.


#### Current algorithms used

In version 2.1 work on this heuristic was disabled after I discovered a
surprisingly simple algorithm that solves all these problems. First I
decided to exactly define the “best approximation”. This should be done
by defining a price for every approximation and choose the approximation
with the lowest price. Prices are defined as such:

Approximating row/column x by y costs *dist(x, y) ^ 2*.

This prefers two smaller approximation errors before a single larger
error and describes my goal quite well.

The cost for adding a new row/column specifies when it is better to do a
bad approximation and when to add a new row/column. I use *(4 \* step)
\* (4 \* step)*. This means that the approximation is acceptable when
*dist(x, y) \< 4 \* step*. Otherwise, adding a new row/column costs
less. Now the best approximation is known. All that is required is a
fast algorithm to do this. Surprisingly, this is possible in linear time
using a relatively simple dynamic algorithm. It uses approximations of
*length \< n* to make a guess at the length of *n*. It can start by
approximating one row/column and then again for two, three up to
xsize/ysize rows/columns.

The algorithm starts by calculating prices for all possible new
positions for old row/column 1. Because of the pricing there are
maximally 8 new positions. (Other ones must cost more than adding new
row/column). Of course it is possible that there are no new positions.

For calculating the price of approximations for row/column 2 I may use
the previous column: Try new position n. Calculate the price and add the
best approximation for the previous (row/column 1) that uses a new
position lower than n (thus prohibiting doubling or swapping). This
should be one of 8 positions or (eventually) adding a new one and not
using row/column 1 at all.

The same method can be used for the rest of the rows/columns. At the end
the best price may be found for the last row/column and return by the
way it was calculated. (For this I need the saved “calculated using”
values.) At this step the best approximation has been determined.

To fill the table, *9 \* n* steps are required and n steps to backtrack
to the best approximation. The only problem is that this algorithm is
still a little slow (chiefly because of slow memory access on the Intel
architectures). But, with some optimizing, it works well.

This algorithm is almost perfect except that it occasionally adds new
rows/columns to the wrong locations — it does not prefer to add new
rows/columns into holes — but it does not seem that this is a real
problem. The last optimization made was based upon the fact that added
rows/columns do not have the exact real and imaginary components
calculated by (*beginning + x \* step*) but lie at the average of their
left and right neighbors. This makes the boundaries smooth and
distributes coordinates better. It also has the added benefit of making
the input better for future approximations.

Another danger during implementation of this algorithm is that adding
new rows/columns into their ideal positions could cause misordered
results, since some rows/columns could be off more than the distance
between them. To avoid this, I use an algorithm that always examines the
start and end of a block of new rows/columns and linearly interpolates
the value between them. Special care needs to be taken with the blocks
that start at the beginning or finish at the end.

Implementation should be much faster using custom fixed-point routines —
first recalculate values such that 0 means start of image and 65536
means end. Than calculation is much cleaner. Values \<0 and \>65536 are
off screen, calculation is independent of scale, and many things should
be recalculated — like tables for calculating price from distance. Also
dividing the main loops into many specialized parts and avoiding filling
unnecessary parts of tables helps. So current algorithm in XaoS is about
5 or 6 times faster than first naive implementation.


### Moving Pixels to New Positions

Since XaoS is using the approximation algorithm the following table is
filled for every row/column:

  - calculate
  - oldpoint
  - position

calculate is 1 if the current row/column is new and needs to be
calculated or 0 if no old pixels need to be moved. oldpoint is a pointer
to the old row/column that corresponds to the new one. This pixel needs
to be copied to the new location. position is the real and imaginary
components of the coordinates used for future approximations. Because
almost all points will be moved, the solution seems to be simple: for
every new point look at the row and column table; copy it if required.

There is the problem that this minimally needs three memory reads for
every pixel (read calculate, oldpoint and index of old point). This is
too slow, so a small optimization is performed. Instead of rewriting the
piece of code in assembly, normal memcpy is used to move blocks of
pixels to their new locations. This minimizes the internal loop and
access can be done more quickly since memcpy is usually optimized for
each architecture.

Using the row table, a list of blocks to move for every row is created.
With this new table all the pixels can be moved quickly. This increased
the speed of XaoS by about four times and made this function so fast
that it is no longer a problem. (In fact, it takes much less time than
all other parts of XaoS.)


### Calculating New Pixels

The above optimizations make XaoS very fast, but another 30% increase in
speed is acquired by using a clever method for calculating the new
pixels. Many methods are known for saving calculations during the
generation of fractal images. The most powerful is boundary detection.
It relies on the fact that the Mandelbrot Set is connected with lakes.
You need only one pixel at the boundary, then can traverse the whole set
and fill the solid area inside. This method saves many calculations but
is too complex for adding just one line. Many claim that it does not
introduce any errors, but this is not true. It is possible for a
connected part of the lake to be so small that it is not visible in
smaller resolutions. In this case, boundary detection misses the whole
area. This algorithm is actually used just for calculating of new images
(i.e. at the startup).

XaoS uses modification of method known as solid guessing. The pixels at
the boundaries of a rectangle are calculated. If they are all the same
you may assume that this rectangle does not does not contain anything
and fill it.

This algorithm is further modified to operate on added lines. For this
it is at least as good as boundary detection and produces more tangible
errors. When adding a single line, the upper and lower line may be
examined for the nearest three pixels. If they are all the same then it
is assumed that 9x9 pixels are the same. This disables all calculations
inside solid areas and calculates as many points as boundary detection.
The only possibility of creating a larger error with this method as
opposed to boundary detection is in the instance that the shape of the
set is so sharp that it does not set any of the tested points but comes
from the right (i.e., uncalculated) location. This situation is not very
common.

Later, rules were added for new rows and columns that crossed each
other. In this instance you can test only four pixels. This situation is
very rare. It is hoped that it does not introduce many errors.

If multiple blocks of new lines need to be calculated there are no
reference pixels to use for solid guessing. Interlacing does the trick.
By calculating the odd lines without any guessing, the guessing
algorithm is now possible for the remaining uncalculated lines. This
simple trick saves about 30% of the calculation of the main Mandelbrot
image.

A similar approximation can also be done for the X coordinate. This
makes it possible to improve solid guessing at even pixels because all
surrounding pixels are available, further reducing errors.


### Symmetry

Many fractals are horizontally or vertically symmetrical. This is
implemented in the approximation code. When there is no good
approximation available, try to mirror the opposite side if the line is
available.

This method primarily speeds up the initial image.


### Calculation of the Mandelbrot Set

The internal Mandelbrot calculation loop is unrolled — it calculates the
first 8 iterations using the normal method, and then it expects that
number of iterations will probably be large, so it switches into a mode
where it calculates iterations in blocks of 8 with one bailout test at
the end. When the bailout is attained, saved values from previous
iterations are restored and the last 8 iterations are recalculated
slowly to get exact values. This especially helps on the Pentium, where
conditionals in floating point code are slow.

Another optimization is periodicity checking. XaoS has loops with and without
periodicity checks. In most cases it uses the no-periodicity-checking
version. The periodicity check version is used just in the case where
some inside-set pixel has been found during the solid guessing phase.
This is done mainly because the periodicity checking version of the loop
is significantly slower.


### Dynamic Resolution

The above optimizations often do not help enough and image calculation
is still too slow. One option was to reduce the framerate, but a
framerate lower than 5 frames per second is unbearable. Another option
is simply to calculate only the details that can be determined within a
time interval.

Rows/columns not calculated are simply approximated by referencing the
nearest row/column. The result is an image with larger pixels. One
problem is the fact that the order of calculating the rows/columns is
significant. Previous versions of XaoS simply calculated all rows from
top to bottom and then columns from left to right. Using the dynamic
resolution code with this algorithm would result in distorted images.
This was solved by adding a priority to every row/column and calculating
the high priority row/column first. The algorithm for adding these
priorities is as follows:

  - Find middle row/column of uncalculated block. Priority is the size
    of the block (in floating point coordinates)
  - Start function for left block and right block

This function produces quite good results. It tends to make same-sized
rectangles on the whole image and does not depend on resolution.

Another interesting optimization is that during the zoom it is more
advantageous to calculate rows/columns in the center of the zoom instead
of the borders since these will be in the viewport longer and the user
is usually focusing on the center of the zoom anyhow.

This is done by simply adding to the calculated priority
*normal\_priority / (abs(newposition - oldposition) / step + 1)*. This
prefers rows/columns that do not move a great deal. (Of course,
unzooming uses the reverse of this formula.)

The last variable to consider is the time interval for one frame.
Setting it too low makes the calculation slow. Setting it too high makes
the framerate too low. So the amount of time spent in other parts of the
program is calculated and multiplied by 5 to determine the interval. If
this indicates a framerate lower than 15FPS, 15FPS is used instead,
since slower animations are unacceptable. On the other hand, if it is
higher than 35FPS, it is set to 35FPS, since a higher framerate than
that is just wasting computer resources. When the image is not
animating, this value is changed, so a framerate between 5FPS and 15FPS
is selected. This ensures that images are calculated quickly after
zooming stops.


### Autopilot

Another interesting algorithm controls the autopilot. It is actually
quite simple. Interesting parts are found at the boundaries of the set.
It randomly looks around and zooms to the first area containing both
outside and inside set points. Some fractals (such as the Newton) do not
have points inside the set at all. In this case it selects a point where
many (more than 2) different colors are around. (i.e., It zooms into
noisy areas.)

In the instance that there are no such areas, the autopilot will unzoom.
It also detects oscillations / vacillations and breaks them.

The current implementation also does detection of out of range numbers;
randomly chosen points are chosen near the old one, to avoid frequent
changes of direction.
