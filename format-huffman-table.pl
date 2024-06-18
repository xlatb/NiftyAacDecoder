#!/usr/bin/perl -w
use warnings;
use strict;

use Data::Dumper;
use IO::File;

if (scalar(@ARGV) != 4)
{
  STDERR->printf("Usage: %s <file.txt> <signed|unsigned> <dimension> <maxabsval>\n", $0);
  exit(1);
}

my $filename = $ARGV[0];

my $signed = {'signed' => 1, 'unsigned' => 0}->{$ARGV[1]};
(!defined($signed)) && die("Unknown value for 'signed': '$ARGV[1]'");

my $dimension = int($ARGV[2]);

my $maxabsval = int($ARGV[3]);
my $mod = ($signed) ? (($maxabsval * 2) + 1) : ($maxabsval + 1);

my $infile = IO::File->new();
$infile->open($filename, 'r') || die("open(): '$filename': $!");

# Read records
my $recs = [];
while (my $line = $infile->getline())
{
  ($line =~ m#^\s*$#) && next;

  $line =~ s#^\s+##;
  $line =~ s#\s+$##;
  my ($index, $bits, $codeword) = split(/\s+/, $line);

  push(@{$recs}, {index => $index, bits => $bits, codeword => hex($codeword)});
}

# Decode indices into an array of values
for my $rec (@{$recs})
{
  my $index = $rec->{index};
  $rec->{values} = [];

  for (my $i = 0; $i < $dimension; $i++)
  {
    my $v = $index % $mod;
    ($signed) && do { $v -= $maxabsval; };
    
    unshift(@{$rec->{values}}, $v);
    $index = int($index / $mod);
  }
}

# Sort
$recs = [ sort({ ($a->{bits} <=> $b->{bits}) || ($a->{codeword} <=> $b->{codeword}) } @{$recs}) ];

$infile->close();

printf("{\n");
printf("  .count = %d,\n", scalar(@{$recs}));
printf("  .maxBits = %d,\n", $recs->[-1]->{bits});
printf("  .entries =\n");
printf("  {\n");
for my $rec (@{$recs})
{
  printf("    {%2d, 0x%-6X", $rec->{bits}, $rec->{codeword});

  for (my $i = 0; $i < $dimension; $i++)
  {
    printf(", %2d", $rec->{values}->[$i]);
  }
  printf("},\n");
}
printf("  }\n");
printf("};\n");

#print main::Dumper($recs);
