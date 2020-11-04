# Myar Archiving Program

- System calls: read, write, lseek, stat, utime, unlink
- library routines: directory routines, string routines
- umask

myar [qxotvdA:] archive-file [file1...]

myar operates on the archive-file listed adding, deleting, listing contents according to the
operations selected, using the files specified as appropriate. Myar maintains compatibility
with “ar” utility on Linux, with the exception of the “A” option which is an added option to
myar, and any item mentioned in the notes below.

-q Quickly add at the bottom the files specified to the archive-file \
-x Extract the specified files from the archive-file \
-o Used in combination with “x” restore the orginal permission and mtime \
-t List the file names in the archive-file \
-v Used in combination with “t” print the verbose info as “ar” does. \
-d Delete the files specified from the archive-file \
-A Quickly append all regular files in the directory modified more than N days ago