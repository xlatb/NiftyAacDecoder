#!/usr/bin/perl -w
use warnings;
use strict;

use Data::Dumper;
use IO::File;

if (scalar(@ARGV) != 1)
{
  STDERR->printf("Usage: %s <file.txt>\n");
  exit(1);
}

my $filename = $ARGV[0];

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
  printf("    {%2d, 0x%-6X, 0x%-2X},\n", $rec->{bits}, $rec->{codeword}, $rec->{index});
}
printf("  }\n");
printf("};\n");

#print main::Dumper($recs);
