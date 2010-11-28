#!/usr/bin/perl -w
# procress the output from refcount.gdb
# supress known ref-pairs
#
# grep "g_object_unref" refcount.txt | wc -l
# grep "g_object_ref" refcount.txt | wc -l
#

@blocks=();
@refold=();
@refnew=();
@refcor=();
$ix=-1;
$fix=0;
$cor=0;

%stats=();

# read log file
# build chunks from "Hardware watchpoint" to "Hardware watchpoint"
open(LOGFILE, "refcount.log") or die("Could not open input log file.");
foreach $line (<LOGFILE>) {
    # do line-by-line processing.
    if ($line =~ m/^Hardware watchpoint/) {
        $ix++;
    	$blocks[$ix]=[];
    	$refcor[$ix]=0;
    } elsif ($line =~ m/^Old\ value\ =\ *(\d+)$/) {
    	$refold[$ix]=$1;
    } elsif ($line =~ m/^New\ value\ =\ *(\d+)$/) {
    	$refnew[$ix]=$1;
    } elsif ($line =~ m/^#\d+\ +/) {
        chomp $line;
        # filter addresses
        $line =~ s/^(#\d+\ +)(0x[0-9,a-fA-F]+ in )?(.*)$/$3/;
        # drop glibs "IA__" prefixes
        $line =~ s/IA__//g;
    	push @{$blocks[$ix]},$line;
    }
}
close(LOGFILE);

# filter known patterns
for($i=0;$i<=$ix;$i++) {
pattern_filter_loop:
    $filter=0;
    for($j=0;$j<scalar(@{$blocks[$i]});$j++) {
        $frame=$blocks[$i][$j];
        if($frame =~ m/g_object_set_valist \(/) {
            # - filter if prev line is g_value_object_collect_value/g_value_unset
            $frame2=$blocks[$i][$j-1];
            if ($frame2 =~ m/g_value_(object_collect_value|unset) \(/) {
                $filter=1;
                $skey="g_object_set_valist";
                last;
            }
        } elsif($frame =~ m/object_set_property \(/) {
            # - filter if prev line is g_value_transform/g_value_unset
            $frame2=$blocks[$i][$j-1];
            if ($frame2 =~ m/g_value_(transform|unset) \(/) {
                $filter=1;
                $skey="object_set_property";
                last;
            }
        } elsif($frame =~ m/g_object_get_valist \(/) {
            # - filter the ref/unref for the object we run the get on,
            #   but not if object is a property we _get!
            # - filter if the previous frame is !object_get_property
            $frame2=$blocks[$i][$j-1];
            if ($frame2 !~ m/object_get_property \(/) {
                $filter=1;
                $skey="g_object_get_valist";
                last;
            }
        } elsif($frame =~ m/g_object_new_valist \(/) {
            # - filter if prev line is g_value_object_collect_value/g_value_unset
            $frame2=$blocks[$i][$j-1];
            if ($frame2 =~ m/g_value_(object_collect_value|unset) \(/) {
                $filter=1;
                $skey="g_object_new_valist";
                last;
            }
        } elsif($frame =~ m/gtk_tree_model_get \(/) {
            # - does a ref,ref,unref
            #   - remove the ones that do: g_value_set_object/g_value_unset
            for($k=$j;$k>=0;$k--) {
                $frame2=$blocks[$i][$k];
                if ($frame2 =~ m/g_value_(set_object|unset) \(/) {
                    $filter=1;
                    last;
                }
            }
            if($filter==1) {
                $skey="gtk_tree_model_get";
                last;
            }
        } elsif($frame =~ m/gtk_list_store_set \(/) {
            # - does a ref,ref,unref
            #   - remove the ones that do: g_value_object_collect_value/g_value_unset
            for($k=$j;$k>=0;$k--) {
                $frame2=$blocks[$i][$k];
                if ($frame2 =~ m/g_value_(object_collect_value|unset) \(/) {
                    $filter=1;
                    last;
                }
            }
            if($filter==1) {
                $skey="gtk_list_store_set";
                last;
            }
        } elsif($frame =~ m/g_signal_emit_valist \(/) {
            $filter=1;
            for($k=$j;$k>=0;$k--) {
                $frame2=$blocks[$i][$k];
                if ($frame2 !~ m/g_/) {
                    $filter=0;
                    last;
                }
            }
            if($filter==1) {
                $skey="g_signal_emit_valist";
                last;
            }
        #} elsif ($frame =~ m/gst_object_dispatch_properties_changed \(/) {
        #    $filter=1;
        }
    }
    if($filter==1) {
        $frame=$blocks[$i][1];
        $skey.=".pat";
        if($frame =~ m/g_object_ref \(/) {
            if(!defined($stats{$skey}{"ref"})) {
                $stats{$skey}{"ref"}=0;
            }
            $stats{$skey}{"ref"}++;
            $cor++;
        } elsif($frame =~ m/g_object_unref \(/) {
            if(!defined($stats{$skey}{"unref"})) {
                $stats{$skey}{"unref"}=0;
            }
            $stats{$skey}{"unref"}++;
            $cor--;
        }
        # delete entry
        splice(@blocks, $i, 1);
        splice(@refold, $i, 1);
        splice(@refnew, $i, 1);
        $ix--;
        $fix++;
        goto pattern_filter_loop;
    } else {
        $refcor[$i]=$cor;
    }
}

# look for pairs where we do ref followed by unref
# - there is a g_object_get|gtk_tree_model_get|... in frame[i], that leads to the ref
# - the next backtrace contains the same callstack below frame[i]
#   and frame[i] is the unref here
for($i=0;$i<=$ix;$i++) {
localref_filter_loop:
    $filter=0;
    for($j=0;$j<scalar(@{$blocks[$i]});$j++) {
        $frame=$blocks[$i][$j];
        if($frame =~ m/((g_object|gtk_tree_model)_get) \(/) {
            $skey=$1;
            $frame2=$blocks[$i][$j+1] =~ m/^(.*) \(/;
            $func1=$1;
            for($k=0;$k<scalar(@{$blocks[$i+1]});$k++) {
                $frame2=$blocks[$i+1][$k] =~ m/^(.*) \(/;
                $func2=$1;
                if ($func1 eq $func2) {
                    $frame2=$blocks[$i+1][$k-1];
                    if($frame2 =~ m/g_object_unref \(/) {
                        $filter=1;
                        last;
                    }
                }
            }
        }
    }
    if($filter==1) {
        $skey.=".loc";
        if(!defined($stats{$skey}{"ref"})) {
            $stats{$skey}{"ref"}=0;
        }
        $stats{$skey}{"ref"}++;
        if(!defined($stats{$skey}{"unref"})) {
            $stats{$skey}{"unref"}=0;
        }
        $stats{$skey}{"unref"}++;
        # delete entry
        splice(@blocks, $i, 2);
        splice(@refold, $i, 2);
        splice(@refnew, $i, 2);
        splice(@refcor, $i, 2);
        $ix-=2;
        $fix+=2;
        goto localref_filter_loop;
        
    }
}

# look for pairs that we can't easily match, but unequal counts could indicate problems
# gtk_list_store_set & gtk_list_store_remove


#truncate traces
truncate_loop:
$last=$blocks[0][scalar(@{$blocks[0]})-1];
$truncate=1;
for($i=1;$i<=$ix;$i++) {
    $frame=$blocks[$i][scalar(@{$blocks[$i]})-1];
    if($frame ne $last) {
      $truncate=0;
      last;
    }
}
if($truncate==1) {
    for($i=0;$i<=$ix;$i++) {
        pop @{$blocks[$i]};
    }
    goto truncate_loop;
}

# dump reformatted
open(LOGFILE, ">refcount.txt") or die("Could not open output log file.");
for($i=0;$i<=$ix;$i++) {
    if($refcor[$i] == 0) {
      printf(LOGFILE "%d -> %d\n",$refold[$i],$refnew[$i]);
    } else {
      printf(LOGFILE "%d -> %d (%d -> %d)\n",$refold[$i]-$refcor[$i],$refnew[$i]-$refcor[$i],$refold[$i],$refnew[$i]);
    }
    $j=0;
    foreach $line (@{$blocks[$i]}) {
        printf(LOGFILE "  #%2d %s\n",$j++,$line);
    }
}
close(LOGFILE);

# statistics

$total=$ix+$fix;
print "Splitted $total backtraces into $ix custom entries and $fix filtered entries\n";

foreach $skey (keys(%stats)) {
    $sref=$stats{$skey}{"ref"};
    $sunref=$stats{$skey}{"unref"};
    printf "%25s : %3d ref, %3d unref : %s\n",$skey,$sref,$sunref,($sref==$sunref)?"==":"!=";
}
