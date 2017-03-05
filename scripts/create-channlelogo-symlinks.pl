#!/usr/bin/perl

if (not defined $ARGV[0]) {die "No parameter given! \ncreate-symlinks.pl <channels.conf> <logos> <link-dir>\n"};
if (not defined $ARGV[1]) {die "No parameter given! \ncreate-symlinks.pl <channels.conf> <logos> <link-dir>\n"};
if (not defined $ARGV[2]) {die "No parameter given! \ncreate-symlinks.pl <channels.conf> <logos> <link-dir>\n"};

my $inpath0 = "$ARGV[1]";
my $linkpath = "$ARGV[2]";

%png0 = %png1 = ();

# png folder einlesen
opendir DIR, "$inpath0" or {die " cant open $inpath0" };

while(my $file = readdir DIR) {
    if( $file =~ /\.png$/) {
	$value = $file;
	$file =~ /(.*).png/;
	$key = $1;
	$key =~ s/\W//g;
	$png0{$key} = $value;
    }
}
closedir DIR;


open LOG, ">>translate.log" or die "Can't open log file!\n";

# channels.conf einlesen
open (FILE, "< $ARGV[0]") or die "Can't open file\n";
while (<FILE>) {
    $channame = $shortname = '';
    $line = $_;
    $line =~ s/\r\n//;
    if ($line =~ /^:/ or $line =~ /^@/ ) { next; }

    @line = split(/:/, $line);
    $line[0] =~ s/\'//;
    $line[0] =~ s/\///;
    if ($line[0] =~ m/;/) { $line[0] =~ /(.*);.*/; $line[0] = $1 }

    if ($line[0] =~ m/,/) { 
	@names = split(/,/, $line[0]);
	$channame = $names[0]; $shortname = $names[1];
    }
    else { $channame = $line[0]; $shortname = ''; }

    if ($channame eq '' or $channame eq '.') { next; }
    
    $searchname = $channame;
    $searchname =~ s/\W//g;
    $searchname =~ tr/[A-Z]/[a-z]/;
    
    if ($png0{$searchname}) {
	$cnt++;
	$status = symlink("./../$inpath0/$png0{$searchname}","$linkpath/$channame.png");
	if ($status == 1)  { print LOG "$channame => ./../$inpath0/$png0{$searchname}"; }
	else { print LOG "$channame => failed"; } 
	if ($shortname and $shortname ne '') {
	    $status = symlink("./../$inpath0/$png0{$searchname}","$linkpath/$shortname.png");
	    if ($status == 1)  { print LOG "\t$shortname"; }
	    else { print LOG "\t$shortname => failed"; } 
	}
	print LOG "\n"; next;
    }
    elsif ($shortname and $shortname ne '') {
	
	$searchname = $shortname;
	$searchname =~ s/\W//g;
	$searchname =~ tr/[A-Z]/[a-z]/;
    
	if ($png0{$searchname}) {
	    $cnt++;
	    $status = symlink("./../$inpath0/$png0{$searchname}","$linkpath/$shortname.png");
	    if ($status == 1)  { print LOG "$channame => ./../$inpath0/$png0{$searchname}"; }
	    else { print LOG "$shortname => failed"; } 
	    if ($channame and $channame ne '') {
		$status = symlink("./../$inpath0/$png0{$searchname}","$linkpath/$shortname.png");
		if ($status == 1)  { print LOG "\t$channame"; }
	    else { print LOG "\t$channame => failed"; }
	    }
	    print LOG "\n"; next;
	}
    }

    $searchname = $channame;
    $searchname =~ s/\W//g;
    $searchname =~ tr/[A-Z]/[a-z]/;
    
    if ($png1{$searchname}) {
	$cnt++;
	$status = symlink("./../png1/$png1{$searchname}","$linkpath/$channame.png");
	if ($status == 1)  { print LOG "$channame => ./../png1/$png1{$searchname}"; }
	else { print LOG "$channame => failed"; } 
	if ($shortname and $shortname ne '') {
	    $status = symlink("./../png1/$png1{$searchname}","$linkpath/$shortname.png");
	    if ($status == 1)  { print LOG "\t$shortname"; }
	    else { print LOG "\t$shortname => failed"; } 
	}
	print LOG "\n"; next;
    }
    elsif ($shortname and $shortname ne '') {
	
	$searchname = $shortname;
	$searchname =~ s/\W//g;
	$searchname =~ tr/[A-Z]/[a-z]/;
    
	if ($png1{$searchname}) {
	    $cnt++;
	    $status = symlink("./../png1/$png1{$searchname}","$linkpath/$shortname.png");
	    if ($status == 1)  { print LOG "$channame => ./../png1/$png1{$searchname}"; }
	    else { print LOG "$shortname => failed"; } 
	    if ($channame and $channame ne '') {
		$status = symlink("./../png1/$png1{$searchname}","$linkpath/$channame.png");
		if ($status == 1)  { print LOG "\t$shortname"; }
		else { print LOG "\t$channame => failed"; } 
	    }
	    print LOG "\n"; next ;
	}
    }
}
close(FILE) or die "Can't close file\n";
close(LOG) or die "Can't close file\n";

print $cnt, "\n";
