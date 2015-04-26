#!/usr/bin/perl -w


# Read in the filter list.
# Print out lines starting with + directly.
# Save the lines starting with - in the array @del for deleting
# the files from input later
# Save the lines starting with ! in the array @gen for transforming
# the files in input later
#
open FILTER, "<manifest-filter.lst";
while (<FILTER>) {
    /^\+ *(.*)$/ and print "$1\n";
    /^- *(.*)$/ and push @del, $1;
    /^! *([^ ]+) +(.*)$/ and push @gen, { pat => $1, rep => $2};
}

# Loop through the standard input, deleting lines that match regexes
# in @del and performing any matching transformations.
@lines = <>;
LINE: foreach (@lines) {
    for $f (@del) {
        /$f/ and next LINE;
    }
    $l = $_;
    for $f (@gen) {
      %h = %$f;
      $r =$l;
      if ($r =~ /$h{pat}/) {
        eval "\$r =~ s\@$h{pat}\@$h{rep}\@";
        push @lines, $r;
      }

    }
    print $l;
}

