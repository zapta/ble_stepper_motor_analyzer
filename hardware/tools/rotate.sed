#!/usr/bin/sed -f

# coordinate transforms (for pretty much everything)
s/(\(start\|end\|at\) \(-\?[0-9.]\+\) \(-\?[0-9.]\+\))/(\1 \3 -\2)/g
s/(\(at\) \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) \(-\?[0-9.]\+\))/(\1 \3 -\2 \4)/g

# dimension transforms (for things like SMD pads)
s/(\(size\) \(-\?[0-9.]\+\) \(-\?[0-9.]\+\))/(\1 \3 \2)/g

# we don't need no double negatives :)
s/--\([0-9.]\+\)/\1/g

# turn -0 into 0
s/-0\([^0-9.]\+\)/0\1/g

# rotate text
s/(fp_text \(.*\) (at \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) 270)/(fp_text \1 (at \2 \3 360)/g
s/(fp_text \(.*\) (at \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) 180)/(fp_text \1 (at \2 \3 270)/g
s/(fp_text \(.*\) (at \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) 90)/(fp_text \1 (at \2 \3 180)/g
s/(fp_text \(.*\) (at \(-\?[0-9.]\+\) \(-\?[0-9.]\+\))/(fp_text \1 (at \2 \3 90)/g
s/(fp_text \(.*\) (at \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) 360)/(fp_text \1 (at \2 \3)/g

# if present, rotate 3D models 
# (note that positive rotation here is clockwise, not counterclockwise)
s/(rotate (xyz \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) 0))/(rotate (xyz \1 \2 -90))/g
s/(rotate (xyz \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) 90))/(rotate (xyz \1 \2 0))/g
s/(rotate (xyz \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) 180))/(rotate (xyz \1 \2 90))/g
s/(rotate (xyz \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) 270))/(rotate (xyz \1 \2 180))/g
s/(rotate (xyz \(-\?[0-9.]\+\) \(-\?[0-9.]\+\) -90))/(rotate (xyz \1 \2 270))/g


