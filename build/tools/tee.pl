#! /usr/bin/perl
#
# modified from https://gist.github.com/wh13371/5de280d5a0ba0fb45649#file-tee-pl
# usage: ping 8.8.8.8 | tea.pl [-a] [-t] [FILE]
# -a = append | -t = prefix timestamp to output | FILE = output filename

use IO::Handle;
use Getopt::Long;
use Time::Piece;
use Time::HiRes;
use POSIX;

sub cmd_args
{
 GetOptions("append|a", \$append, "time|t", \$time) or die $!;
}

sub now
{ 
 my ($secs, $us) = Time::HiRes::gettimeofday;
 my $dt = strftime(q/%Y-%m-%d %H:%M:%S./, localtime($secs)) . sprintf("%06d", $us);
 return $dt;
}

sub echo
{
 print @_;
 print OUT @_;
}

sub main
{
 $f = $ARGV[0] || "tea." . $^T . ".log"; # create an epoch padded filename if ARGV is null

 open OUT, $append ? '>>' : '>', $f || die "$!\n";
 OUT->autoflush(1);

 my $t0 = time;
 
 while (<STDIN>) 
 {
  echo /\S/ && $time ? now() . ": " . $_ : $_;
 }

 echo "Elapsed: " . Time::Seconds->new(time - $t0)->pretty . "\n";

 close OUT;
}

cmd_args;

main;

exit;
