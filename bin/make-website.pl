#!@PERL@ -w 
# -*-Perl-*- 


# stupid script to generate WWW site.  The WWW site is my 
# test-suite for LilyPond, I usually don't distribute versions that
# fail to make the website

use FileHandle;
use Getopt::Long;

my $lily_version;
my $footstr;
my $mw_id = "<!make_website!>";
my $id_str = "make-website 0.4";
my $TAR="tar";
my $MAKE="make";

sub get_version
{
    my ($vstr)=("");
    open V, "$depth/VERSION";
    while (<V>) {
	s/#.*$//g;
	next if (/^ *$/);
	s/^/\$/;
	s/= *(.*)$/=\"$1\";/;
	$vstr .= $_;
    }
    eval ($vstr);
    
    $lily_version= "$TOPLEVEL_MAJOR_VERSION.$TOPLEVEL_MINOR_VERSION.$TOPLEVEL_PATCH_LEVEL$TOPLEVEL_MY_PATCH_LEVEL";
    
    # stupid checks.
    $lily_version= $lily_version;
    
    close V;
}

sub set_html_footer
{
    my $MAILADDRESS=$ENV{MAILADDRESS};
    my @pw=(getpwuid($<));
    my $username=$pw[6];

    $footstr = 
	"\n<hr>Please take me <a href=index.html>back to the index</a>\n
of LilyPond -- The GNU Project Music typesetter
<hr>
<font size=-1>
This page was built using <code>" . $id_str . "</code> from lilypond-"
    . $lily_version . 
	" by<p>
<address><br>$username <a href=mailto:" 
    . $MAILADDRESS . ">&lt<!bla>" . $MAILADDRESS ."</a>&gt</address>
<p></font>";
}
    

# do something, check return status
sub my_system
{
    my (@cmds) = @_;
    foreach $cmd (@cmds) {
	my ($ignoreret)=0;
	if ( $cmd  =~ /^-/ ) {
	    $ignoreret = 1;
	    $cmd = substr ($cmd, 1);
	}
	
	my $ret =  ( system ($cmd));
	if ($ret) {
	    if ($ignoreret) {
		print STDERR "ignoring failed command \`$cmd\' (status $ret)\n";
	    }else {
		print STDERR "\nmake_website: failed on command \`$cmd\' (status $ret)\n";
		exit 2;
	    }
	}
    }
}


local $base="lilypond/";

local @examples=("twinkle-pop", 
		 "wtk1-fugue2",
		 "standchen-16", 
		 "standchen-20", 
		 "wtk1-prelude1",
		 "toccata-fuga-E", 
		 "scsii-menuetto",
		 "cadenza", 
		 "gallina",
		 "twinkle", 
		 "collisions",
		 "font",
		 #"scales", 
		 "rhythm", 
		 "multi" );


sub gen_html
{
    print "generating HTML\n";
    my_system "$MAKE -kC .. html";
}

sub gen_examples
{
    print "generating examples: \n";
    my @todo=();
    foreach $a (@examples) {
	push @todo, "out/$a.ps.gz", "out/$a.gif", "out/$a.ly.txt";
    }
    
    my_system ("$MAKE -C .. " . join(' ', @todo));
}

my @texstuff = ("mudela-man", "mudela-course");

sub gen_manuals
{
    print "generating TeX doco list\n";
    open HTMLLIST, ">tex_manuals.html";
    print HTMLLIST "<HTML><TITLE>PostScript Manuals</TITLE>\n" ;
    print HTMLLIST "<BODY><h1>LilyPond manuals (in PostScript)</h1>";
    print HTMLLIST "<ul>\n";
    my @todo=();
    foreach $a (@texstuff) {
	push @todo , "out/$a.ps.gz";
	print HTMLLIST "<li><a href=$a.ps.gz>$a.ps.gz</a>";
    }
    print HTMLLIST "</ul>";
    
    print HTMLLIST "</BODY></HTML>";
    close HTMLLIST;

    my_system( "$MAKE -C .. " .  join(' ', @todo));
}

sub gen_list
{
    print "generating HTML list\n";
    open HTMLLIST, ">example_output.html";

    print HTMLLIST "<html><body><TITLE>Rendered Examples</TITLE>\n
These example files are taken from the LilyPond distribution.
LilyPond currently only outputs TeX and MIDI. The pictures and
PostScript files were generated using TeX, Ghostscript and some
graphics tools.  The papersize used for these examples is A4.  The GIF
files have been scaled to eliminate aliasing."; 



    foreach $a (@examples)
    {
	$name=$a; print HTMLLIST "<h1>example file: $name</h1>\n<XMP>\n";

	open IF, "$depth/input/$a.ly";
	input_record_separator IF "\n}";
	
	$desc = <IF>;
	close IF;
	
	print HTMLLIST "$desc\n</XMP>";

	$inputf="$a.ly.txt";
	$giff="$a.gif";
	$jpegf="$a.jpeg";
	$pngf = "$a.png";
	$psf="$a.ps.gz";
	$midif="$a.midi";
	
	print HTMLLIST "<ul>";

	print HTMLLIST "<li><a href=$inputf> The input file</a>"
	    if ( -f $inputf );
	
	print HTMLLIST "<li><a href=$giff>The output (picture)</a>"	
	    if ( -f $giff );

	print HTMLLIST "<li><a href=$psf>The output (PS)</a>\n"
	    if ( -f $psf );
		
	print HTMLLIST "<li><a href=$midif>The output (MIDI)</a>\n" 
	    if ( -f $midif );
	print HTMLLIST "</ul>";
    }
    print HTMLLIST "</BODY></HTML>";
    close HTMLLIST;
}

sub edit_html
{
    print STDERR "adding footer\n";

    OUTER:
    foreach $a (<*.html>) {
	open H, "$a";
	my $sep="</BODY>";
	input_record_separator H $sep;
	my $file="";
	
	while (<H>) { 
	    if (/$mw_id/) {
		close H;
		next OUTER;
	    }
	    $file .= $_; 

	}
	close H;

	my $subst =  $footstr;
	$subst .= $back if (! $a =~ /index.html/ );
	$file =~ s/$sep/$subst$sep/g ;
	$file =~ s/\.gif/\.$image/g;
	$file =~ s!<TITLE>(.*)</TITLE>!<TITLE>LilyPond WWW: $1</TITLE>!g;
	open H, ">$a";
	print H $mw_id;
	
	print H $file;
	close H;
    }
}

sub copy_txt_file
{
    my ($f) = @_;
    my $d = $f;
    $d =~ s!^.*\/!!;
    if (! $f =~ /.txt$/) {
	$d = "$f.txt";
    }
    print, $d;
}
    
sub top_of_NEWS
{
    open NEWS, "NEWS.txt";
    input_record_separator NEWS "****";
    $desc = <NEWS>;
    chop ($desc);
    close NEWS;

    return $desc;
}

sub edit_index
{
    $ton = top_of_NEWS();
    $ton = "\n<XMP>\n$ton\n</XMP>\n";
    open INDEX, "index.html";
    input_record_separator NEWS undef;
    $index = <INDEX>;
    close INDEX;
    $index =~ s/top_of_NEWS/$ton/;
    open INDEX, ">index.html";
    print INDEX $index;
    close INDEX;
}


sub copy_files
{  
    print "copying files\n";
    my_system "ln -s $depth/out ./docxx" if ( ! -x "docxx" ) ;
    my_system "cp $depth/TODO ./TODO.txt",
    "cp $depth/ANNOUNCE ./ANNOUNCE.txt",
    "cp $depth/NEWS ./NEWS.txt",
    "cp $depth/DEDICATION ./DEDICATION.txt";
    my_system "make -C .. gifs";
    
}

sub set_images
{
    for $a (<*.gif>) {
	if ($opt_png) {
	    my_system "gif2png -d $a";
	}
	if ($opt_jpeg) {
	    my $b=$a;
	    $b =~ s/.gif/.jpeg/;
	    my_system "cjpeg -o $b $a";
	}
    }
}

sub docxx_update
{
    open BANNER, ">/tmp/lilybanner.html";
    my $ban = $footstr;
    $ban =~ s!index.html!../index.html!g;
    print BANNER $ban;
    close BANNER;
    my_system("BANNEROPT=\"-B /tmp/lilybanner.html\" $depth/bin/out/make-docxx");
}

sub do_tar
{
     print "tarring.\n";
     $files = join (' ', < *.html *.$image *.ps.gz *.txt *.midi docxx/*>);
     my_system
	 "-$TAR zvhcf website.tar.gz $files;",
#	 "gzip -f9 website.tar;";
}

sub identify
{
    print STDERR "This is " . $id_str . "\n";
    
}
sub main
{
    identify;
    GetOptions("jpeg", "gif", "png", "noexamples");

    local $image="gif" ;
    $image = "png" if ($opt_png);
    $image = "jpeg" if ($opt_jpeg);
    
    $depth = "../";
    my $cwd;
    chomp($cwd = `pwd`);
    die "need to be in directory Documentation\n" if ( ! ($cwd =~ /Documentation$/));
    get_version;
    print "lily v. " . $lily_version . "\n";
    set_html_footer;


    $depth = "../../";

    chdir ("out");
    $ENV{"TEXINPUTS"} .= ":$depth/input/:";
    $ENV{"LILYINCLUDE"} = "$depth/input/";
    $ENV{"LILYTOP"} = $depth;

    gen_html;
    copy_files;

    if (! $opt_noexamples) {
	gen_examples;
	gen_list;
	gen_manuals;
    }
    set_images;

    edit_html;
    edit_index;
    docxx_update;
    do_tar;
}

main;
