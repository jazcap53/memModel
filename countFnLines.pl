#!/usr/bin/perl

while (<>) {
    chomp;
    if (/(^\w+ )([a-zA-Z ]*::)(\w+\()/) {
        if ($1 ne 'using ') {
            $functionName = $1 . $2 . $3 . ")";
            $ctr = 0;
            while ($line = (<>)) {
                $ctr++;
                if ($line =~ /^}/) {
                    last;
                }
            }
            print "\t" . $functionName . " has " . $ctr . " lines\n";
        }
    }
}
