#!/usr/bin/perl -w

# script to create HTML-pages and diagrams from the glest-factions techtree
# 20110120, bugs & feedback to olaus@rupp.de
# license: GPLv3 or newer

our $version = "0.1 alpha";
# this code is far from perfect, just published on popular demand ;)

# general comments for running:
# - developed on ubuntu linux 10.04 and with modules from ubuntu (install with "apt-get install libgraphviz-perl libconfig-inifiles-perl libxml-xpath-perl")
# - adjust the paths in the .ini-file to your system and run with "convert_faction_xml2html.pl example.ini"
# - won't work on windows without adjustments (search for "system", at least ... ) and i guess some of the modules used aren't easily available for active-state-perl

# general comments for changing the code:
# - as far as possible i used "pidgin-perl" and tried to avoid complicated functions and data-structures. so hopefully anybody with bare programming skills can adopt this code to his needs
# - it runs with "use strict" and -w without any warnings. this makes coding a bit more complicated but saves lots of time in the long run, get used to it ;)
# - watch out for singular & plural in the variable-names, $harvested_resources{} ne $harvested_resource
# - if you need to read some new stuff from the XML-files, google the xpath-documentation. check if it's single- or multi-valued. use xpath.pl to check the location.
# - if you want to rewrite this in your favorite language i recommend your local xpath-module, then you can reuse the xpath-locations from this script

# TODO:
# - requirement page per faction with all buildings and producing units and the units they produce (footnotes for requirements) (collect in $production{ $u } and $production_units{ $u }
# - table of armor-types vs attack-types (f.e. megapack.xml )

# maybe someday ...
# - generate some descriptive text for each unit, like "this is the unit with the most hitpoints/damage to air units/armor/movement speed/... of the tech faction" 
# - diff feature (CGI?) to show what changed in units if moving f.e. from vanila glest 3.2 to MG 3.4
# - make overview tables sortable (javascript?)
# - overview tables without border but different colors in odd/even rows
# - list of units this one is strong/weak against (via armor- and attack-types)
# - top-lists over all factions (units with most hit points, damage, ...)
# - calculate a "bang for the buck" for 10 units including all cost for upgrades, buildings, ...
# - multilanguage (use more icons, lng-files but unit names aren't multilanguage anyway)
# - import bitmap-icons in SVG (doesn't work with perls graphviz-module, would need to parse and change SVG)
# - map symbology below diagram
# - single pages for units with animated gif, link to XML, mini-diagram of techtree just for this unit

# usefull for converting HTML-colors: http://www.yafla.com/yaflaColor/ColorRGBHSL.aspx

# strange stuff in the techtrees:
# - persian magician splash radius 0?
# - tech workers move faster with load?
# - tech upgrades piercing/blade weapon don't work for battle machine?
# - f.e. battle machine with "hold" will only attack land units

####################################################################

my $cfg;
my $cfg_file;

if ( $cfg_file = shift @ARGV ) {
	
	if ( ! -e $cfg_file ) {
		die "cfg_file not existing: $cfg_file\n";
	}
}
else {
	$cfg_file ="glest.ini";
}

$cfg = new Config::IniFiles( -file => "./$cfg_file" );


use FindBin;
use File::Spec;
use Cwd;
BEGIN {
    $ENV{APP_ROOT} = Cwd::realpath(File::Spec->rel2abs($FindBin::Bin)) ;
}

our $use_relative_paths = $cfg->val( 'all', 'relative_paths');

our $debug = $cfg->val( 'all', 'debug');

our $glest_version = $cfg->val( 'all', 'version');

our $factions_path=$cfg->val( 'files' , 'factions_path');
$factions_path = "$ENV{APP_ROOT}/$factions_path" if ( $use_relative_paths );
our $resources_path=$cfg->val( 'files' , 'resources_path');
$resources_path = "$ENV{APP_ROOT}/$resources_path" if ( $use_relative_paths );
our $out_path=$cfg->val( 'files' , 'out_path');
$out_path = "$ENV{APP_ROOT}/$out_path" if ( $use_relative_paths );

our $units_path=$cfg->val( 'files' , 'units_path');
our $upgrades_path=$cfg->val( 'files' , 'upgrades_path');
our $images_path=$cfg->val( 'files' , 'images_path');

our $glestkeys=$cfg->val( 'files' , 'glestkeys');
$glestkeys = "$ENV{APP_ROOT}/$glestkeys" if ( $use_relative_paths );

our $summary = $cfg->val('files', 'summary');

our $svg_fontcolor = $cfg->val( 'style', 'svg_fontcolor');


our @resource_order = split(/:/, $cfg->val( 'style', 'resource_order') );

my $now_string = localtime;
my $me = $0;
$me =~ s/^.+\///;

our $footer=$cfg->val( 'style', 'footer');
our $map_legend = $cfg->val('style', 'map_legend');

$footer =~ s/VAR_CREATED_BY/$me using config-file $cfg_file on $now_string /;



use strict;
use XML::XPath;
use XML::XPath::XMLParser;
use GraphViz;
use Config::IniFiles;

$|=1;

if ( ! -e $factions_path ) {
	die "ERROR: cant find factions_path: $factions_path, check $cfg_file\n";
}

if ( ! -e $resources_path ) {
	die "ERROR: cant find resources_path: $resources_path, check $cfg_file\n";
}

mkdir $out_path;
mkdir "$out_path/images";
if ( ! -w $out_path ) {
	die "cant find out_path: $out_path or no write permission\n";
}

#our @factions  = <$factions_path/magi*>;
#our @factions  = <$factions_path/tec*>;
#our @factions  = <$factions_path/egy*>;
our @factions  = <$factions_path/*>;

our @resources = <$resources_path/*>;

our @all_html_pages;

our %icon;
our %max_hp;
our %max_ep;
our %regeneration;
our %regeneration_ep;
our %building;
our %armor;
our %armor_type;
our %sight;

our %upgrade;
our %upgrade_effects;
our %move_speed;
our %production_speed;

our %worker;
our %combat;
our %air_unit;
our %land_unit;

our %created_by_unit;
our %c_unit_allows;
our %c_unit_requires;
our %c_all;
our %c_resource_requirement;
our %c_resource_store;
our %c_harvested_resources;
our %max_load;
our %hits_per_unit;
our %c_unit_requirements;
our %c_upgrade_requirements;

our %graph_files;
our %c_nodes;
our %c_edges;
our %c_edges_from;
our %edges_relation;
our %multi_hop;
our %multi_hop_upgrade;

# for storing skills:
our %c_skill;
our %c_command;
our %speed;
our %ep_cost;
our %skill_type;
our %attack_range;
our %attack_strenght;
our %attack_var;
our %attack_type;
our %attack_start_time;
our %attack_land;
our %attack_air;
our %splash;
our %splash_radius;
our %splash_damage_all;

# for storing commands:
our %commands_of_unit;
our %command_type;
our %command_icon;
our %morph_skill;
our %morph_unit;
our %morph_discount;

our %move_skill;
our %move_loaded_skill;
our %attack_skill;
our %repair_skill;
our %build_skill;
our %produce_skill;
our %upgrade_skill;
our %harvest_skill;

our %produce_unit;
our %c_produced_units;
our %c_repaired_units;

our $graph_all;
our $graph_buildings;
our $graph_buildings_units;

our %tips;

# create page with all keys
if ( -f $glestkeys ) {
	&create_glestkey_page();
}

if ( $cfg->val('files', 'load_tips') ) {
	&load_tip_files();
}


################################################
# built HTML for selecting faction
our $all_faction_links ="";
our $all_faction_links_overview ="";
foreach my $faction_path ( @factions ) {
	my $faction = $faction_path;
	$faction =~ s/.+\///;
	my $faction_pretty = &format_name( $faction );
	$all_faction_links .= "&nbsp;<A HREF=${faction}_techtree.html>$faction_pretty</a> |&nbsp;";
	$all_faction_links_overview .= "&nbsp;<A HREF=${faction}__overview.html>$faction_pretty</a> |&nbsp;";
}
# remove last pipe
$all_faction_links =~ s/ \|&nbsp;$//;
$all_faction_links_overview =~ s/ \|&nbsp;$//;


################################################
# copy resource images
mkdir "$out_path/images";
mkdir "$out_path/images/resources";
foreach my $resource_path ( @resources ) {
	my $resource = $resource_path;
	$resource =~ s/.+\///;
	system "cp $resource_path/images/$resource.bmp $out_path/images/resources";
}

################################################
# read all units and draw graphs
foreach my $faction_path ( @factions ) {

	my $faction = $faction_path;
	$faction =~ s/.+\///;
	my $faction_pretty = &format_name( $faction );

	#my $graph = GraphViz->new( bgcolor => "0.5666,0,0.086", layout => "twopi" , overlap => "false");
	#my $graph = GraphViz->new( bgcolor => "0.5666,0,0.086", layout => "fdp", concentrate => 1 , overlap => 0, epsilon => 0.01 );

	$graph_all             = GraphViz->new( bgcolor => "0.5666,0,0.086", layout => "dot", rankdir => 0 , overlap => 'false', concentrate => 1, epsilon => 0.01 );
	# layout twopi, doesn't "bend" edges around nodes so the result doesn't look as good as it could
	#$graph_all             = GraphViz->new( bgcolor => "0.5666,0,0.086", layout => "twopi", rankdir => 0 , overlap => 'false', concentrate => 0, epsilon => 0.01 );
	$graph_buildings       = GraphViz->new( bgcolor => "0.5666,0,0.086", layout => "dot", rankdir => 0 , overlap => 'false', concentrate => 1, epsilon => 0.01 );
	$graph_buildings_units = GraphViz->new( bgcolor => "0.5666,0,0.086", layout => "dot", rankdir => 0 , overlap => 'false', concentrate => 1, epsilon => 0.01 );

	# good, from left to right:, not exactly 16:9 ;)
	#my $graph = GraphViz->new( bgcolor => "0.5666,0,0.086", layout => "dot", rankdir => 1 , overlap => 0 );



	print ("doing faction: $faction in path: <$faction_path>/<$units_path>\n") if ($debug);

	mkdir ("$out_path/$images_path");
	mkdir ("$out_path/$images_path/${faction}");

	#system "cp $faction_path/loading_screen.jpg $out_path/$images_path/${faction}";

	# read XML-files for all units
	chdir ("$faction_path/$units_path");
	opendir(UNITS, "." ) || die "cant open directory $faction_path/$units_path\n";
	my @units = grep { !/\./ } readdir(UNITS);
	closedir(UNITS);

	foreach my $unit ( @units ) {

		push @{ $c_all{"$faction"} }, $unit;

		print "reading unit: $faction:$unit in $faction_path\n" if ( $debug );
		my $unit_file ="$faction_path/$units_path/$unit/$unit.xml";

		my ( $icon ) = &read_unit( $unit_file, $faction, $faction_path, $unit);

		mkdir ("$out_path/$images_path/${faction}");
		system "cp $faction_path/$units_path/$unit/images/$icon $out_path/$images_path/${faction}";

	}

	# read XML-files for all upgrades
	chdir ("$faction_path/$upgrades_path");
	opendir(UPGRADES, "." ) || die "cant open directory $faction_path/$upgrades_path\n";
	my @upgrades = grep { !/\./ } readdir(UPGRADES);
	closedir(UPGRADES);

	foreach my $upgrade ( @upgrades ) {

		push @{ $c_all{"$faction"} }, $upgrade;

		print "reading upgrade: $faction:$upgrade in $faction_path\n" if ( $debug );
		my $upgrade_file ="$faction_path/$upgrades_path/$upgrade/$upgrade.xml";

		my ( $icon ) = &read_unit( $upgrade_file, $faction, $faction_path, $upgrade );

		system "cp $faction_path/$upgrades_path/$upgrade/images/$icon $out_path/$images_path/${faction}";

	}

	# don't clutter the diagram with direct links (mostly "requirement") if there's an indirect link with 2 hops. 
	# f.e. don't show a link that the technodrom is a requirement of the defense-tower because there's a link from
	# technodrom to advanced architecture and a link from there to defense tower.
	&check_multi_hops($faction);

	# now the graphviz-nodes are set, do the edges
	foreach my $edge ( @{$c_edges{"$faction"}} ) {
		
		my ( $unit_from, $unit_to, $style, $relation ) = split(/:/, $edge );

		print "dedge: deciding edge for $faction:$unit_from:$unit_to - $relation -style: $style\n";


		# only show requirement-link of there's no build, morph or produce link
		# otherwise graphviz would show a dotted line where a solid one makes more sense
		if ( 
			(
				$relation eq "Requirement"  ||
				$relation eq "Upgrade-Requirement"  
			) && (
				$edges_relation{"$faction:$unit_from:$unit_to:Build"} ||
				$edges_relation{"$faction:$unit_from:$unit_to:Morph"} ||
				$edges_relation{"$faction:$unit_from:$unit_to:Produce"} 
			)
		) {
		}
		else {

			my $dont_link_in_all=0;
			if ( 
				$multi_hop{"$faction:$unit_from:$unit_to"}       &&
				$relation ne "Build"                             &&
				$relation ne "Morph"                             &&
				$relation ne "Produce"
			) {
				$dont_link_in_all=1;
				print "dedge: not showing edge $faction:$unit_from:$unit_to\n";
			}


			if ( $relation eq "Requirement"  && $building{"$faction:$unit_to"} ) {
				$graph_all             -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor ) if (!$dont_link_in_all);
				$graph_buildings       -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
			}
			elsif ( 
				$relation eq "Requirement"           ||
				$relation eq "Upgrade-Requirement"  
			) {
				$graph_all             -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor ) if (!$dont_link_in_all);
				print "dedge: SHOWING edge $faction:$unit_from:$unit_to\n";
				#$graph_buildings_units -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
			}
			elsif ( $relation eq "Build" ) {
				$graph_all             -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor ) if (!$dont_link_in_all);
				$graph_buildings_units -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
				#$graph_buildings       -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
			}
			elsif ( $relation eq "Upgrade" ) {
				$graph_all             -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor ) if (!$dont_link_in_all);
				##$graph_buildings_units -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
				##$graph_buildings       -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
			}
			elsif ( $relation eq "Morph" ) {
				$graph_all             -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor ) if (!$dont_link_in_all);
				$graph_buildings_units -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
				##$graph_buildings       -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
			}
			elsif ( $relation eq "Produce" ) {
				$graph_all             -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor ) if (!$dont_link_in_all);
				$graph_buildings_units -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
				##$graph_buildings       -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
			}
			else {
				die "shouldnt happen, grep me 545454545: $relation\n";
			}
		}
	}

	# create edges in diagrams which don't show the upgrades but go via an upgrade
	# f.e. edge from technodrome to defense tower via advanced architecture
	# TODO
	# doesn't work because %multi_hop doesn't contain a link from technodrome to defense tower because there's no direct link
	# so it's not found by &check_multi_hops()
	foreach my $multihop ( keys %multi_hop_upgrade ) {
		my ( $faction, $unit_from, $unit_to ) = split(/:/, $multihop );

		print "found multihop from building to building $unit_from to $unit_to\n";
		my $style="dotted";
		$graph_buildings_units -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
		$graph_buildings       -> add_edge("$unit_from" => "$unit_to", style => $style, color => $svg_fontcolor );
	}

	&export_graphs($faction);

	if ( 
		$cfg->val('all', 'build_clickable_map') &&
		$cfg->val('all', 'export_graph') =~ /as_png/ &&
		$cfg->val('all', 'export_graph') =~ /as_cmapx/
	) {
		&build_clickable_map( $faction );
	}
}

####################################################################################
# now that we have all the unit- and upgrade-requirements: export HTML
####################################################################################
foreach my $faction_path ( @factions ) {

	my $faction = $faction_path;
	$faction =~ s/.+\///;

	my $faction_pretty = &format_name( $faction );

	my $html_page = "${faction}_techtree.html";
	my $outfile = "$out_path/$html_page";
	open (HTML, "> $outfile") || die "can't write outfile: $outfile\n";
	push @all_html_pages, $html_page;


	my $full_all = "<P>&nbsp;<P><HR><H1>Unit Details for Faction $faction_pretty</H1>\n";
	my $overview_all   = "<TABLE BORDER=1 WIDTH=\"100%\"><TR> <TH WIDTH=200> Name 
						<TH> Total Cost
						<TH> Hit Points 
						<TH> Regenerate 
						<TH> Armor Strength 
						<TH> Armor Type 
						<TH> Sight Range 
					";

	my $overview_combat   = "<H3>Combat Units</H3> $overview_all 
					<TH> Move Speed
					<TH> Strength vs. Land 
					<TH> Strength vs. Air 
					<TH> Attack Range
				";

	my $overview_worker   = "<H3>Worker Units</H3> $overview_all
					<TH> Move Speed
				";

	my $overview_building = "<H3>Buildings</H3> $overview_all
					<TH>Storage
				";

	my $overview_upgrades = "<H3>Upgrades</H3><TABLE BORDER=1><TR> <TH WIDTH=200> Name 
						<TH> Cost
						<TH> Increases
						<TH> Affects
						<TH> Enables to build
					";

	# now we have read all the XMLs, loop again over all units and export them to HTML
	foreach my $unit ( sort @{$c_all{"$faction"}} ) {

		my $full = "";

		my $u = "$faction:$unit";
		print "doing HTML for $u\n" if ( $debug );

		my $full_tmp = "";
		my $found = 0;

		my $unit_pretty = &format_name( $unit );

		# FULL
		$full .= "\n<TABLE BORDER=1>";
		$full .= "<TR><a name=\"${unit}_full\"><TD WIDTH=300>".&html_icon( $u, 64 )."<TD WIDTH=400> <b>$unit_pretty</b>";

		if ( $upgrade{$u} ) {
			$full .= "<TR><TD>Type:<td>Upgrade\n";
		}
		elsif ( $building{$u} ) {
			$full .= "<TR><TD>Type:<td>Building\n";
		}
		else {

			$full .= "<TR><TD>Type:<td>";
			if ( $worker{$u} ) {
				$full .= "Worker Unit<br>\n";
			}
			# a unit might fight and build so no elsif!
			if ( $combat{$u} ) {
				$full .= "Combat Unit\n";

				
			}
		}

		if ( $tips{ $unit } ) {
			$full .= "<TR><TD>Description:<TD> ".$tips{ "${unit}_info" };
		}

		$full .= "<TR><TD>Creating:<TD>";
		foreach my $created_by_unit_tmp ( @{$created_by_unit{ $u }} ) {

			my ( $created_by_u, $method, $discount ) = split(/:/, $created_by_unit_tmp );

			print "$u created by unit: $created_by_u method: $method\n";


			if ( $method eq "morph" ) {
				$full .= "Morphing from ".&link_unit(${created_by_u})."<BR>\n";
			}
			elsif ( $method eq "produce" ) {
				$full .= "Produced by ".&link_unit(${created_by_u})."<BR>\n";
			}
			elsif ( $method eq "build" ) {
				$full .= "Built by ".&link_unit(${created_by_u})."<BR>\n";
			}
			elsif ( $method eq "upgrade" ) {
				$full .= "Upgraded by ".&link_unit(${created_by_u})."<BR>\n";
			}
		}

		# put cost together

		my $full_tmp_cost;

		my ( $cost, $cost_icon, $cost_icon_overview );
		# loop over creation-methods because f.e. guard might be produced in barracks or upgraded from swordman

		my $num_creation_methods =@{$created_by_unit{ $u }}; 

		foreach my $created_by_unit_tmp ( @{$created_by_unit{ $u }} ) {

			my ( $created_by_u, $method, $discount ) = split(/:/, $created_by_unit_tmp );

			my ( $cost_tmp, $cost_icon_tmp, $cost_icon_overview_tmp ) = &calc_cost_of_unit( $u, $created_by_u, $method, $discount );
			$cost_icon .= $cost_icon_tmp;

			if ( $num_creation_methods == 1 ) {
				$full_tmp_cost .= "<TR><TD>Total Cost: <TD> $cost_icon_tmp\n";
			}
			else {
				# f.e. guard might be produced in barracks or upgraded from swordman
				my $created;
				if ( $method eq "morph" ) {
					$created = "upgraded from";
				}
				elsif ( $method eq "produce" ) {
					$created = "produced by";
				}
				elsif ( $method eq "build" ) {
					$created = "built by";
				}
				elsif ( $method eq "upgrade" ) {
					$created = "upgraded by";
				}
				else {
					print "method: $method for $u $created_by_u\n";
				}
				$full_tmp_cost .= "<TR><TD>Total Cost when ${created} ".&format_name($created_by_u).": <TD> $cost_icon_tmp\n";
			}

			# prefer cost for produce-method in overview if more than one method (guard) because that's less micromanagement for player
			if ( 
				$num_creation_methods == 1 ||
				$num_creation_methods >= 2 &&
				$method eq "produce" 
			) {
				$cost_icon_overview = $cost_icon_overview_tmp;
			}
		}


		# put storage of resources together
		my $storage="";
		my $storage_icon="";
		foreach my $resource_stored ( @{$c_resource_store{"$faction:$unit"}} ) {
			my ( $resource, $amount ) = split(/:/, $resource_stored );

			$storage .= &format_name($resource).":&nbsp;$amount, ";
			$storage_icon .= "<IMG SRC=\"images/resources/$resource.bmp\" HEIGHT=16 WIDTH=16>&nbsp;$amount&nbsp;";
		}
		# remove trailing ", "
		chop $storage;
		chop $storage;
		print "storage: $storage\n";

		# do commands & skills

		my $commands="";

		my $full_attack_tmp="";
		my $full_move_tmp="";
		my $full_morph_tmp="";
		my $full_repair_tmp="";
		my $full_harvest_tmp="";

		my $max_strength_vs_land = 0;
		my $max_strength_vs_land_var = 0;
		my $max_strength_vs_air  = 0;
		my $max_strength_vs_air_var  = 0;
		my $max_range            = 0;
		my $max_move_speed       = 0;

		foreach my $c ( @{$commands_of_unit{"$u:move"}} ) {
			################################################################
			# MOVE-command
			################################################################
			my ( $faction, $unit, $command ) = split(/:/, $c );

			my $skill = $move_skill{ $c };
			my $s = "$faction:$unit:$skill";

			$full_move_tmp .= "<TR><TD> Move Command: ".&format_name($command)." <TD> Speed: ".$speed{ $s }."<BR>\n";

			if ( $speed{ $s } > $max_move_speed ) {
				$max_move_speed = $speed{ $s };
			}

		}

		my $skill_on_hold_position="";

		# don't ask me why this doesn't work ???????????
		#$num_of_attacks = @{$commands_of_unit{"$u:attack"}};
		# so do workaround:
		my $num_of_attacks = 0;
		foreach my $x ( @{$commands_of_unit{"$u:attack"}} ) {
			$num_of_attacks++;
		}

		foreach my $c ( @{$commands_of_unit{"$u:attack_stopped"}} ) {

			my ( $faction, $unit, $command ) = split(/:/, $c );
			my $command_pretty = &format_name( $command );

			$commands .= "$command_pretty, ";

			# defensive-buildings like defense tower don't have an attack skill but just an attack_stopped skill
			# so show this as main attack
			if ( !$num_of_attacks ) {
				my $full_attack_tmp_tmp;

				( $full_attack_tmp_tmp, $max_strength_vs_land, $max_strength_vs_land_var, $max_strength_vs_air, $max_strength_vs_air_var, $max_range, $max_move_speed ) = 
					&show_attack( $c, $max_strength_vs_land, $max_strength_vs_land_var, $max_strength_vs_air, $max_strength_vs_air_var, $max_range, $max_move_speed );

				$full_attack_tmp .= $full_attack_tmp_tmp;
			}
			else {
				$skill_on_hold_position= $attack_skill{ $c };
				if ( $c_upgrade_requirements{ $c } ) {
					print "skill requires: $c_upgrade_requirements{ $c }\n";
					$skill_on_hold_position .= ";$c_upgrade_requirements{ $c }";
				}
			}


		}

		# do all commands with <type value=attack>
		foreach my $c ( @{$commands_of_unit{"$u:attack"}} ) {

			my ( $faction, $unit, $command ) = split(/:/, $c );
			my $command_pretty = &format_name( $command );
			$commands .= "$command_pretty, ";


			my $full_attack_tmp_tmp;

			( $full_attack_tmp_tmp, $max_strength_vs_land, $max_strength_vs_land_var, $max_strength_vs_air, $max_strength_vs_air_var, $max_range, $max_move_speed ) = 
				&show_attack( $c, $max_strength_vs_land, $max_strength_vs_land_var, $max_strength_vs_air, $max_strength_vs_air_var, $max_range, $max_move_speed, $skill_on_hold_position );

			$full_attack_tmp .= $full_attack_tmp_tmp;


		}

	
		# morphing
		foreach my $c ( @{$commands_of_unit{"$u:morph"}} ) {
			my ( $faction, $unit, $command ) = split(/:/, $c );

			my $skill = $morph_skill{ $c };
			my $s = "$faction:$unit:$skill";
			my $skill_short = $skill;
			$skill_short =~ s/_skill$//;

			print "morph_command: $command $c\n";

			$full_morph_tmp .= "<TR><TD> Morph Skill: ".&format_name($skill_short)." <TD>";

			# since f.e. the technician can morph into different units, find the one for this skill
			foreach my $command ( @{$c_command{ $u }} ) {
				my $c = "$u:$command";
				print "comparing command: $skill of $c\n";
				if ( $morph_skill{ $c } && $morph_skill{ $c } eq $skill ) {
					print "fitting morph_skill and command $skill of $c\n";
					$full_morph_tmp .= "Morphing to: ".&link_unit($morph_unit{ $c }).&html_icon($faction.":".$morph_unit{ $c }, 64)."<BR>\n";
					$full_morph_tmp .= "Refund (Discount): ".$morph_discount{ $c }."&nbsp;%<BR>\n";
					$full_morph_tmp .="Speed: ".$speed{ $s }."<BR>\n";
				}
			}
			
		}

		# repairing
		foreach my $c ( @{$commands_of_unit{"$u:repair"}} ) {
			my ( $faction, $unit, $command ) = split(/:/, $c );

			my $skill = $repair_skill{ $c };
			my $s = "$faction:$unit:$skill";
			my $skill_short = $skill;
			$skill_short =~ s/_skill$//;

			print "repair_command: $command $c\n";
			$full_repair_tmp .= "<TR><TD> Repair/Heal Skill: ".&format_name($skill_short)." <TD>".&format_name($skill_short)."ing: ";

			foreach my $uu ( @{$c_repaired_units{$c}} ) {
				$full_repair_tmp .= &link_unit($uu).", ";
			}
			chop $full_repair_tmp;
			chop $full_repair_tmp;
			$full_repair_tmp .="<BR>\n";
	
			$full_repair_tmp .="Speed: ".$speed{ $s }."<BR>\n";
			
		}

		# harvesting/mining
		foreach my $c ( @{$commands_of_unit{"$u:harvest"}} ) {
			my ( $faction, $unit, $command ) = split(/:/, $c );

			my $skill = $harvest_skill{ $c };
			my $s = "$faction:$unit:$skill";
			my $skill_short = $skill;
			$skill_short =~ s/_skill$//;

			print "harvest_command: $command $c\n";
			$full_harvest_tmp .= "<TR><TD> Harvest/Mine Skill: ".&format_name($skill_short)." <TD>";

			foreach my $resource ( @{$c_harvested_resources{$c}} ) {
				$full_harvest_tmp .= &html_icon_resource( $resource, 32 );
			}

			$full_harvest_tmp .="<BR>\n";
	
			$full_harvest_tmp .="Speed: ".$speed{ $s }."<BR>\n";
			$full_harvest_tmp .="Max Load: ".$max_load{ $c }."<BR>\n";
			$full_harvest_tmp .="Hits per Unit: ".$hits_per_unit{ $c }."<BR>\n";
			
		}





		# finnished with commands & skills
		##########################################################


		# first do name & costs, thats the same for all
		my $overview_tmp = "<TR>";
		$overview_tmp .= "<TD> ".&html_icon($u, 32).&link_unit($unit);

		$overview_tmp .= "<TD>$cost_icon_overview\n";
		$full .= $full_tmp_cost;

		if ( $storage ) {
			$full .= "<TR><TD>Storage:<td>$storage_icon\n";

			if ( $storage =~ /wood/i && $storage !~ /gold/i ) {
				$full .= "(Hint: This also means a worker can drop of his harvested wood into this building so it's useful to build it near trees.)";
			}
		}


		if ( !$upgrade{$u} ) {
			# OVERVIEW
			$overview_tmp .= "<TD ALIGN=CENTER>".$max_hp{$u};
			$overview_tmp .= "<TD ALIGN=CENTER>".($regeneration{$u} || "-");
			$overview_tmp .= "<TD ALIGN=CENTER>".($armor{$u} || "-");
			$overview_tmp .= "<TD ALIGN=CENTER>".&format_name($armor_type{$u});
			$overview_tmp .= "<TD ALIGN=CENTER>".($sight{$u} || "-");

			$full .= "<TR><TD>Maximum Hitpoints:<td>".$max_hp{$u}."\n";
			$full .= "<TR><TD>Regeneration of Hitpoints:<TD>".($regeneration{$u} || "-")."\n";
			$full .= "<TR><TD>Maximum Energy Points:<td>".$max_ep{$u}."\n" if ( $max_ep{$u});
			$full .= "<TR><TD>Regeneration Energy Points:<TD>".$regeneration_ep{$u}."\n" if ( $regeneration_ep{$u});
			$full .= "<TR><TD>Armor-Strength:<TD>".($armor{$u} || "-")."\n";
			$full .= "<TR><TD>Armor-Type:<TD>".&link_attack_and_armor_types($armor_type{$u})."\n";
			$full .= "<TR><TD>Sight-Range:<TD>".($sight{$u} || "-")."\n";

		}
		else {
			# handle upgrades
			print "handling upgrade $u - $production_speed{$u} - $armor{$u}\n";

			my $upgrade_benefits="";
			$upgrade_benefits .= "<TR><TD>Increase Hitpoints: <TD>+".$max_hp{$u}."\n" if ( $max_hp{$u} );
			$upgrade_benefits .= "<TR><TD>Increase Energy-Points: <TD>+".$max_ep{$u}."\n" if ( $max_ep{$u} );
			$upgrade_benefits .= "<TR><TD>Increase Sight Range: <TD>+".$sight{$u}."\n" if ( $sight{$u} );
			$upgrade_benefits .= "<TR><TD>Increase Attack Strength: <TD>+".$attack_strenght{$u}."\n" if ( $attack_strenght{$u} );
			$upgrade_benefits .= "<TR><TD>Increase Attack Range: <TD>+".$attack_range{$u}."\n" if ( $attack_range{$u} );
			$upgrade_benefits .= "<TR><TD>Increase Armor Strength: <TD>+".$armor{$u}."\n" if ( $armor{$u} );
			$upgrade_benefits .= "<TR><TD>Increase Move Speed: <TD>+".$move_speed{$u}."\n" if ( $move_speed{$u} );
			$upgrade_benefits .= "<TR><TD>Increase Production Speed: <TD>+".$production_speed{$u}."\n" if ( $production_speed{$u} );

			$full .= $upgrade_benefits;
			$upgrade_benefits =~ s/<TR>//gi;
			$upgrade_benefits =~ s/<TD>//gi;
			$upgrade_benefits =~ s/Increase //gi;
			$overview_tmp .= "<TD ALIGN=CENTER>&nbsp;".$upgrade_benefits;

			$overview_tmp .= "<TD ALIGN=CENTER>&nbsp;";

			my $units="";
			foreach my $unit_tmp ( @{$upgrade_effects{$u}} ) {
				$units .= &link_unit($unit_tmp).", ";
			}
			chop ($units);
			chop ($units);
			$overview_tmp .= ($units || "")."\n";

			$full .= "<TR><TD>Affects:<TD> ".($units || "")."\n" if ( $units );
		}

		my $req_overview_tmp;
		for my $relation ('Build', 'Produce', 'Upgrade', 'Requirement', 'Upgrade-Requirement', 'Command' ) {

			$found = 0;

			if ( $relation =~ /Require/i ) {
				$full_tmp = "<TR><TD>$unit_pretty is an $relation for:<TD>";
			}
			elsif ( $relation =~ /Command/i ) {
				$full_tmp = "<TR><TD>$unit_pretty enables commands:<TD>";
			}
			else {
				$full_tmp = "<TR><TD>$unit_pretty is able to $relation:<TD>";
			}
	
			my $last_command="";
			foreach my $unit_allows ( @{$c_unit_allows{"$faction:$unit:$relation"}} ) {
				my ( $faction, $req_unit, $command ) = split(/:/, $unit_allows );
				my $req_unit_pretty = &format_name( $req_unit );
				$found = 1;

				if ( $command ) {
					$full_tmp .= &format_name($command)." : ".&link_unit($req_unit)."<br>\n";
					if ( $command ne $last_command ) {
						$req_overview_tmp .= "Enables command <i>".&format_name($command)."</i> for: ";
					}
					$req_overview_tmp .= &link_unit($req_unit).", ";
					$last_command=$command;
				}
				else {
					$full_tmp .= &link_unit($req_unit)."<br>\n";
					$req_overview_tmp .= &link_unit($req_unit).", ";
				}
			}

			if ( $found ) {
				print "found enables for $u\n";
				$full .= $full_tmp;
				chop $req_overview_tmp;
				chop $req_overview_tmp;
				$req_overview_tmp = "<TD>$req_overview_tmp";
			}

		}

		$req_overview_tmp = $req_overview_tmp || "<TD>&nbsp;";



		# print what's needed to build this unit and what is enabled to built by this unit
		$found = 0;
		$full_tmp = "<TR><TD>Needed to build $unit_pretty:<TD>";
		foreach my $unit_requirement ( @{$c_unit_requires{"$faction:$unit"}} ) {
			my ( $faction, $req_unit ) = split(/:/, $unit_requirement );
			my $req_unit_pretty = &format_name( $req_unit );
			$full_tmp .= &link_unit($req_unit)."<br>\n";
			$found = 1;
		}

		if ( $found ) {
			$full .= $full_tmp;
		}


		if ( $building{$u} ) {
			$overview_building .= $overview_tmp;
			$overview_building .= "<TD>$storage_icon\n";

		}
		elsif ( $upgrade{$u} ) {
			$overview_tmp .= $req_overview_tmp;
			$overview_upgrades .= $overview_tmp;
		}

		if ( $worker{$u} ) {
			$overview_worker .= $overview_tmp;
			$overview_worker .= "<TD ALIGN=CENTER>".($max_move_speed || "--" )."\n";
		}

		if ( $combat{$u} ) {
			$overview_combat .= $overview_tmp;

			$overview_combat .= "<TD ALIGN=CENTER>".($max_move_speed || "--" )."\n";

			$overview_combat .= "<TD ALIGN=CENTER>";
			if ($max_strength_vs_land ) {
				# next line is showing damage as plus/minus, e.g. 300+-150 (which i prefer because otherwise everybody has to calculate the
				# average to tell that 200-400 is stronger than 50-450)
				$overview_combat .= "$max_strength_vs_land&nbsp;+-&nbsp;$max_strength_vs_land_var";

				# next line is showing damage as range, e.g. 150-450
				#$overview_combat .= ($max_strength_vs_land - $max_strength_vs_land_var )."-".($max_strength_vs_land + $max_strength_vs_land_var );
			}
			else {
				$overview_combat .= "--\n";
			}

			$overview_combat .= "<TD ALIGN=CENTER>";
			if ($max_strength_vs_air ) {
				$overview_combat .= "$max_strength_vs_air&nbsp;+-&nbsp;$max_strength_vs_air_var";
				#$overview_combat .= ($max_strength_vs_air - $max_strength_vs_air_var )."-".($max_strength_vs_air + $max_strength_vs_air_var );
			}
			else {
				$overview_combat .= "--\n";
			}

			$overview_combat .= "<TD ALIGN=CENTER>".($max_range || "--" )."\n";
		}

		$full .= $full_move_tmp;
		$full .= $full_morph_tmp;
		$full .= $full_attack_tmp;
		$full .= $full_repair_tmp;
		$full .= $full_harvest_tmp;
		$full .= "</TR>\n";
		$full .= "</TABLE><p>\n";

		$full_all .= $full;

		my $unit_html = "$out_path/${faction}_${unit}_full.html";
		push @all_html_pages, $unit_html;
	
		open (UNIT, "> $unit_html") || die "cant write unit-html: $unit_html\n";
		print UNIT &header("Unit Details for ".&format_name( $unit )." of the $faction_pretty Faction");
		print UNIT $full;
		print UNIT $footer;
		close UNIT;

	}

	$overview_combat   .= "</TABLE>\n";
	$overview_worker   .= "</TABLE>\n";
	$overview_building .= "</TABLE>\n";
	$overview_upgrades .= "</TABLE>\n";
	
	my $overview_html = "<H1>Overview for Faction $faction_pretty</H1>\n\n";
	$overview_html .= $overview_combat;
	$overview_html .= $overview_worker;
	$overview_html .= $overview_building;
	$overview_html .= $overview_upgrades;

	my $title = "$faction_pretty Techtree of Glest - Version $version\n";

	print HTML &header( $title, $faction );

	my $tmp_links = $all_faction_links;
	$tmp_links =~ s/>($faction_pretty)</><b>$1<\/b></;
	print HTML "Choose faction: $tmp_links <p>\n";

	
	my $graph_file;

	for my $type ('all', 'buildings_units', 'buildings') {

		if ( $graph_files{ "$faction:click_map:$type" } ) {
			$graph_file = $graph_files{ "$faction:click_map:$type" };
		}
		elsif ( $graph_files{ "$faction:svg:$type" } ) {
			$graph_file =~ s/.+?\///g;
			$graph_file ="$images_path/$faction/$graph_file";
		}
		$graph_file =~ s/.+\///;
		print HTML "<h3><a href=\"$graph_file\">Techtree Diagram ".&format_name($type)."</a></h3>\n";
	}

	print HTML $overview_html;
	print HTML $full_all;

	print HTML $footer;
	close (HTML);

	# create overview-page for this faction
	$outfile = "$out_path/${faction}_overview.html";
	open (HTML, "> $outfile") || die "can't write faction overview $outfile\n";
	push @all_html_pages, $outfile;

	$title = "$faction_pretty Overview\n";

	print HTML &header( $title, $faction );

	$tmp_links = $all_faction_links_overview;
	$tmp_links =~ s/>($faction_pretty)</><b>$1<\/b></;
	print HTML "Choose faction: $tmp_links <p>\n";
	print HTML "<h3><a href=\"$graph_file\">Techtree Diagram</a></h3>\n";

	print HTML $overview_html;
	print HTML $footer;
	close (HTML);
}


print "creating global summary page\n";

my $outfile = "$out_path/$summary";

open (HTML, "> $outfile") || die "can't write summary page $outfile\n";

print HTML &header("Glest Autodocumentation Summary");
print HTML "<UL>\n";
foreach my $page ( @all_html_pages ) {
	
	$page =~ s/$out_path\///;
	print HTML "<LI><a href=\"$page\">$page</a>\n";
}
print HTML "</UL>\n";
print HTML $footer;
close HTML;

exit 0;

###############################################################################
# end of main(), now subs
###############################################################################

 
sub create_glestkey_page {
	my $html_page = "glestkeys.html";
	push @all_html_pages, $html_page;
	my $outfile = "$out_path/$html_page";
	open (HTML, "> $outfile") || die "can't write $outfile\n";

	print HTML header("Glest Keyboard Assingment");
	print HTML "Note: Keyboard assingment can be changed in glestuserkeys.ini.<p>\n";
	
	print HTML "<TABLE BORDER=1><TH>Name<TH>Key\n";

	open (KEYS, "< $glestkeys") || die "can't read glestkeys: $glestkeys\n";
	while (my $line =<KEYS>) {
		next if ( $line =~ /^;/ );
		if ( $line =~ /(.+)=(.+)/ ) {
			my $name = $1;
			my $key  = $2;

			$name =~ s/^hotkey//gi;

			my $name_pretty = $name;
			# make name pretty, e.g. CameraRotateUp => Camera Rotate Up
			# spaces before uppercase-chars, expect if preceeded by uppercase, then it's an abbreviation like GUI
			$name_pretty =~ s/(?<![[:upper:]])([[:upper:]])/ $1/g;
			# SaveGUILayout => Save GUI Layout
			$name_pretty =~ s/([[:upper:]])([[:upper:]])([[:upper:]])([[:lower:]])/$1$2 $3$4/g;

			# space before numbers:
			$name_pretty =~ s/(\d)/ $1/;

			$key =~ s/^vk//;
			$key =~ s/\'//g;
			
			print HTML "<TR><TD>$name_pretty<TD ALIGN=CENTER>$key\n";
		}
		
	}

	print HTML "</TABLE>\n";
	print HTML $footer;
	close HTML;
	close KEYS;
}

sub read_unit {

	my ($xml_file, $faction, $faction_path, $unit, $graph) = @_;

	my $xpath = XML::XPath->new( $xml_file );

	my $unit_pretty = &format_name ( $unit );

	# index for all assoc arrays
	my $u = "$faction:$unit";
	print "reading XML for $u\n" if ( $debug );

	$upgrade{$u}      = get_value($xpath,  "/upgrade");

	my $resource_requirements = "";

	if ( $upgrade{$u} ) {
		$icon{$u}         = get_value($xpath,  "/upgrade/image/\@path");
		# where to find resource-requirements, used below
                $resource_requirements = $xpath->find("/upgrade/resource-requirements/resource");

                my $effects = $xpath->find("/upgrade/effects/unit");
		foreach my $effected ( $effects->get_nodelist ) {
			XML::XPath::XMLParser::as_string( $effected ) =~ /name=\"(.+?)\"/;
			my $unit_tmp = $1;
			push @{ $upgrade_effects{ $u } }, $unit_tmp;
		}

		$max_hp{$u}           = get_value($xpath, "/upgrade/max-hp" );
		$max_ep{$u}           = get_value($xpath, "/upgrade/max-ep" );
		$sight{$u}            = get_value($xpath, "/upgrade/sight" );
		$attack_strenght{$u}  = get_value($xpath, "/upgrade/attack-strenght" );
		$attack_range{$u}     = get_value($xpath, "/upgrade/attack-range" );
		$armor{$u}            = get_value($xpath, "/upgrade/armor" );
		$move_speed{$u}       = get_value($xpath, "/upgrade/move-speed" );
		$production_speed{$u} = get_value($xpath, "/upgrade/production-speed" );

	}
	else {
		# do units and buildings ...

		$icon{$u}         = get_value($xpath,  "/unit/parameters/image/\@path");
		# where to find resource-requirements, used below
                $resource_requirements = $xpath->find("/unit/parameters/resource-requirements/resource");

		# basic unit stats:
		$max_hp{$u}          = get_value($xpath,  "/unit/parameters/max-hp/\@value");
		$regeneration{$u}    = get_value($xpath,  "/unit/parameters/max-hp/\@regeneration" );
		$max_ep{$u}          = get_value($xpath,  "/unit/parameters/max-ep/\@value");
		$regeneration_ep{$u} = get_value($xpath,  "/unit/parameters/max-ep/\@regeneration" );
		$armor{$u}           = get_value($xpath,  "/unit/parameters/armor/\@value");
		$armor_type{$u}      = get_value($xpath,  "/unit/parameters/armor-type/\@value");
		$sight{$u}           = get_value($xpath,  "/unit/parameters/sight/\@value");

		if ( get_value($xpath,  "/unit/parameters/fields/field[\@value='land']") ) {
			$land_unit{ $u } = 1;
		}
		# no elsif because some units are land & air, f.e. genie
		if ( get_value($xpath,  "/unit/parameters/fields/field[\@value='air']") ) {
			$air_unit{ $u } = 1;
		}

		# decide if it's a worker unit by checking if it can built something
		$building{$u}     = get_value($xpath,  "/unit/skills/skill/name[\@value='be_built_skill']");
		if ( get_value($xpath,  "/unit/skills/skill/type[\@value='build']") ) {
			$worker{ $u } = "1";
		}
		else {
			$worker{ $u } = "0";
		}


		# now the multi-value stuff:

		# skills:
                my $skills_path = $xpath->find("/unit/skills/skill/name/\@value");
		foreach my $skill_xml ( $skills_path->get_nodelist ) {
			XML::XPath::XMLParser::as_string( $skill_xml ) =~ /value=\"(.+?)\"/;
			my $skill = $1;

			# ignore useless skills
			if ( $skill !~ /stop/i && $skill !~ /die/i &&  $skill !~ /loaded/i &&  $skill !~ /be_built/i ) {

				print "skill: $skill\n";
				push @{ $c_skill{ $u } }, $skill;

				# index-var for storing all skills:
				my $s = "$u:$skill";

				$speed{ $s }             = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/speed/\@value");
				$ep_cost{ $s }           = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/ep-cost/\@value");

				$skill_type{ $s } = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/type/\@value");
				print "skill-type: <".$skill_type{ $s }.">\n";
				if ( $skill_type{ $s } =~ /attack/i ) {
					$combat{ $u } = 1;
					$attack_range{ $s }      = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/attack-range/\@value");
					$attack_strenght{ $s }   = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/attack-strenght/\@value");
					$attack_var{ $s }        = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/attack-var/\@value");
					$attack_type{ $s }       = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/attack-type/\@value");
					$attack_start_time{ $s } = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/attack-start-time/\@value");
					$attack_land{ $s }       = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/attack-fields/field[\@value='land']");
					$attack_air{ $s }        = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/attack-fields/field[\@value='air']");
					my $splash               = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/splash/\@value");
					$splash{ $s }         = 1 if ( $splash =~ /true/ );
					if ( $splash{ $s } ) {
						$splash_radius{ $s }     = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/splash/radius/\@value");
						$splash_damage_all{ $s } = get_value($xpath, "/unit/skills/skill/name[\@value='$skill']/parent::*/splash/damage-all/\@value");
					}
				}
			}

			
			
		}

		##########################################
		# COMMANDS:
		##########################################
                my $commands_path = $xpath->find("/unit/commands/command/name/\@value");
		foreach my $command_xml ( $commands_path->get_nodelist ) {
			XML::XPath::XMLParser::as_string( $command_xml ) =~ /value=\"(.+?)\"/;
			my $command = $1;

			print "command: $command\n";
			push @{ $c_command{ $u } }, $command;

			# index-var for storing all skills:
			my $c = "$u:$command";

			my $command_icon_tmp = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/image/\@path");
			$command_icon{ $c } = $command_icon_tmp;
			$command_icon{ $c } =~ s/.+\///;
			if ( $command_icon{ $c }) {
				print "command icon: ".$command_icon{ $c }." for $c\n" ;
				# commented to save time, not used anyway yet
				system "cp $faction_path/$units_path/$unit/$command_icon_tmp $out_path/$images_path/${faction}";
			}

			$move_skill{ $c }        = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/move-skill/\@value");
			$move_loaded_skill{ $c } = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/move-loaded-skill/\@value");
			$attack_skill{ $c }      = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/attack-skill/\@value");
			$repair_skill{ $c }      = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/repair-skill/\@value");
			$build_skill{ $c }       = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/build-skill/\@value");
			$upgrade_skill{ $c }     = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/upgrade-skill/\@value");
			$harvest_skill{ $c }     = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/harvest-skill/\@value");


			$command_type{ $c } = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/type/\@value");
			print "command-type: <".$command_type{ $c }.">\n";

			push @{ $commands_of_unit{ "$u:$command_type{ $c }" } }, $c;

			my $unit_requirements = $xpath->find("/unit/commands/command/name[\@value='$command']/parent::*/unit-requirements/*");
			foreach my $unit_requirement ( $unit_requirements->get_nodelist ) {
				print "req: ".XML::XPath::XMLParser::as_string( $unit_requirement ) . "\n";
				XML::XPath::XMLParser::as_string( $unit_requirement ) =~ /name=\"(.+?)\"/;
				my $req_name = $1;
				print "FFF unit-req: $c of $u - $req_name\n";

				# this can be multivalued, but not used yet so i'll be lazy 
				$c_unit_requirements{ $c } = $req_name;
			}

			my $upgrade_requirements = $xpath->find("/unit/commands/command/name[\@value='$command']/parent::*/upgrade-requirements/*");
			foreach my $upgrade_requirement ( $upgrade_requirements->get_nodelist ) {
				print "req: ".XML::XPath::XMLParser::as_string( $upgrade_requirement ) . "\n";
				XML::XPath::XMLParser::as_string( $upgrade_requirement ) =~ /name=\"(.+?)\"/;
				my $req_name = $1;
				print "FFF-upgrade-req: $c of $u - $req_name\n";

				# this can be multivalued, but not used yet so i'll be lazy 
				$c_upgrade_requirements{ $c } = $req_name;
				# TODO
				push @{ $c_unit_allows{"$faction:$req_name:Command"} }, "$c";
			}


			if ( $command_type{ $c } =~ /morph/i ) {
				print "found morph command: $command\n";
				$morph_skill{ $c }     = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/morph-skill/\@value");

				my $morph_to_unit      = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/morph-unit/\@name");
				$morph_unit{ $c }      = $morph_to_unit;

				$morph_discount{ $c }  = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/discount/\@value");

				# a guard my be produced in a barrack and morphed from a swordman so %created_by_unit has to be multivalued for each unit
				push @{$created_by_unit{ "$faction:$morph_to_unit" } } , "$unit:morph:$morph_discount{ $c }";
			}
			elsif ( $command_type{ $c } =~ /produce/i ) {
				$produce_skill{ $c }     = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/produce-skill/\@value");
				my $produce_unit_tmp     = get_value($xpath, "/unit/commands/command/type[\@value='produce']/parent::*/produced-unit");
				$produce_unit{ $c }      = $produce_unit_tmp;
				push @{ $c_produced_units{ $u } }, $produce_unit_tmp;
			}
			elsif ( $command_type{ $c } =~ /repair/i ) {
				my $repaired_units = $xpath->find("/unit/commands/command/name[\@value='$command']/parent::*/repaired-units/*");
				foreach my $repaired_unit ( $repaired_units->get_nodelist ) {
					XML::XPath::XMLParser::as_string( $repaired_unit ) =~ /name=\"(.+?)\"/;
					push @{ $c_repaired_units{ $c } }, $1;
				}
			}
			elsif ( $command_type{ $c } =~ /harvest/i ) {
				my $harvested_resources = $xpath->find("/unit/commands/command/name[\@value='$command']/parent::*/harvested-resources/*");
				foreach my $harvested_resource ( $harvested_resources->get_nodelist ) {
					XML::XPath::XMLParser::as_string( $harvested_resource ) =~ /name=\"(.+?)\"/;
					push @{ $c_harvested_resources{ $c } }, $1;
				}
				$max_load{ $c }      = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/max-load/\@value");
				$hits_per_unit{ $c } = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/hits-per-unit/\@value");
			}
		}
	}

	# now read $resource_requirements as set above
	foreach my $resource_requirement ( $resource_requirements->get_nodelist ) {
		print "req: ".XML::XPath::XMLParser::as_string( $resource_requirement ) . "\n";
		XML::XPath::XMLParser::as_string( $resource_requirement ) =~ /name=\"(.+?)\" amount=\"(.+?)\"/;
		my $req_name   = $1;
		my $req_amount = $2;
		print "cost: $u $req_name:$req_amount\n";

		if ( $req_amount ) {
			push @{ $c_resource_requirement{ $u } }, "$req_name:$req_amount";

			# sort cows to workers but avoid buildings giving food
			if ( $req_amount < 0 && $req_name eq "food" && !$building{ $u } ) {
				$worker{ $u } = "1";
			}
		}
	}

	# now read resources stored 
	my $resources_stored = $xpath->find("/unit/parameters/resources-stored/resource");
	foreach my $resource_stored ( $resources_stored->get_nodelist ) {
		print "req: ".XML::XPath::XMLParser::as_string( $resource_stored ) . "\n";
		XML::XPath::XMLParser::as_string( $resource_stored ) =~ /name=\"(.+?)\" amount=\"(.+?)\"/;
		my $res_name   = $1;
		my $res_amount = $2;
		print "resource store: $u $res_name:$res_amount\n";

		if ( $res_amount ) {
			push @{ $c_resource_store{ $u } }, "$res_name:$res_amount";

		}
	}


	my $url = "${faction}_techtree.html#${unit}_full";

	# no /g to avoid spreading "shield level 2" in 3 rows
	$unit_pretty =~ s/ /\n/;

	# create graphviz-node
	&g_add_node( $faction, $unit, $unit_pretty, $url, $building{$u}, $upgrade{$u} );

	# definition of 
	# a. where to find requiremtns in the XML-data
	# 1. if it's stored as parent-child or child-parent (REQUIREMENT, COMMAND)
	# 2. which style of arrow should be used in graph (see cpan.org for graphviz-documentation)
	# 3. relation between units (Produce, Build, ...)
	my %req_data;
	$req_data{"/unit/parameters/unit-requirements/*"}                                      = "REQUIREMENT:dotted:Requirement";
	$req_data{"/unit/parameters/upgrade-requirements/*"}                                   = "REQUIREMENT:dotted:Upgrade-Requirement";
	$req_data{"/unit/commands/command/type[\@value='upgrade']/parent::*/produced-upgrade"} = "COMMAND:solid:Upgrade";
	$req_data{"/unit/commands/command/type[\@value='morph']/parent::*/morph-unit"}         = "COMMAND:dashed:Morph";
	$req_data{"/unit/commands/command/type[\@value='produce']/parent::*/produced-unit"}    = "COMMAND:solid:Produce";
	$req_data{"/unit/commands/command/type[\@value='build']/parent::*/buildings/building"} = "COMMAND:bold:Build";

	foreach my $location ( keys %req_data ) {

		my ( $order, $style, $relation ) = split(/:/, $req_data{"$location"} );

                my $unit_requirements = $xpath->find("$location");
                foreach my $unit_requirement ( $unit_requirements->get_nodelist ) {
                        print "req: ".XML::XPath::XMLParser::as_string( $unit_requirement ) . "\n";
                        XML::XPath::XMLParser::as_string( $unit_requirement ) =~ /name=\"(.+?)\"/;
                        my $req_name = $1;

                        # generating an hash of an array
			if ( $order eq "COMMAND" ) {

				&g_add_edge($faction, $unit, $req_name, $style, $relation );

				if ( $relation eq "Morph" ) {
					# dealt with in section skills
				}
				elsif ( $relation eq "Build" ) {
					push @{$created_by_unit{ "$faction:$req_name" } } , "$unit:build";
					push @{ $c_unit_allows{"$u:Build"} }, "$faction:$req_name";
					print "build: $u = $req_name:build\n";
				}
				elsif ( $relation eq "Produce" ) {
					push @{$created_by_unit{ "$faction:$req_name" } } , "$unit:produce";
					push @{ $c_unit_allows{"$u:Produce"} }, "$faction:$req_name";
					print "produce: $u = $req_name:produce\n";
				}
				elsif ( $relation eq "Upgrade" ) {
					push @{$created_by_unit{ "$faction:$req_name" } } , "$unit:upgrade";
					push @{ $c_unit_allows{"$u:Upgrade"} }, "$faction:$req_name";
					push @{ $c_unit_requires{"$faction:$req_name"} }, $u;
					print "upgrade: $u = $req_name:upgrade\n";

					# some commands to upgrade of the egypt faction contain unit-requirements and upgrade-requirements
					# f.e. research_scarab of the temple
					my $command = get_value($xpath, "$location/parent::*/produced-upgrade[\@name='$req_name']/parent::*/name");
					my $c = "$faction:$unit:$command";
					my $req_name_upgrade = $c_upgrade_requirements{ $c };

					if ( $req_name_upgrade ) {
						print  "FFF-found upgrade-req command $c to get $req_name requires: $faction:$req_name_upgrade\n";
						push @{ $c_unit_requires{"$faction:$req_name"} }, "$faction:$req_name_upgrade";
						&g_add_edge($faction, $req_name_upgrade, $req_name, "dotted", "Upgrade-Requirement" );
					}

					my $req_name_unit = $c_unit_requirements{ $c };

					if ( $req_name_unit ) {
						print  "FFF-found unit-req command: $c to get $req_name requires: $faction:$req_name_unit\n";
						push @{ $c_unit_requires{"$faction:$req_name"} }, "$faction:$req_name_unit";
						&g_add_edge($faction, $req_name_unit, $req_name, "dotted", "Requirement" );
					}
				}
				else {
					die "grep me: ererrerrrerere\n";
					#push @{ $c_unit_allows{$u} }, "$faction:$req_name";
					#push @{ $c_unit_requires{"$faction:$req_name"} }, $u;
				}
			}
			elsif ( $order eq "REQUIREMENT" ) {
				&g_add_edge($faction, $req_name, $unit, $style, $relation );
				push @{ $c_unit_allows{"$faction:$req_name:$relation"} }, $u;
				push @{ $c_unit_requires{$u} }, "$faction:$req_name";

			}
			else {
				die "unknown order: $order . grep me <sadsaasdsas\n";
			}
		}
	}


	$icon{ $u } =~ s/^.+\///;

	return ( $icon{$u} );
}

sub header {

	my ( $title, $faction ) = @_;

	$title .= " (".$glest_version.") ";

	my $header = $cfg->val('style', 'header' );
	$header =~ s/VAR_TITLE/$title/g;
	$header =~ s/VAR_SUMMARY/$summary/g;

	return $header;
}


sub format_name {

	# make "shield_level_1" => "Shield Level 1"
        my ( $name ) = @_;

        $name =~ s/_/ /g;
        $name =~ s/(.)/\u$1/;
        $name =~ s/( )(.)/$1\u$2/g;

        return $name;
}

sub get_value {


	my ( $xpath, $location ) = @_;
	my $nodeset = $xpath->find("$location");
	print "doing location $location\n";
	my ($node) = $nodeset->get_nodelist;

	my $value;
	if ( $node ) {
		my $attribute = XML::XPath::XMLParser::as_string( $node );

		# get only the value of an attribute, XML::Xpath returns f.e. regeneration="3", we want just 3
		if ( $attribute =~ /\"(.+?)\"/ ) {
			$value = $1;
		}
		else {
			$value = $attribute;
		}
	}
	return $value;

}


sub g_add_node {

	my ( $faction, $unit, $unit_pretty, $url, $building, $upgrade ) = @_;
	#print "g_add_node: $unit,  <$url>, $building, $upgrade\n";

	push @{ $c_nodes{"$faction"} }, "$unit:$unit_pretty:$url:".($building || "").":".($upgrade || "");

	if ( $building ) {
		$graph_all            -> add_node($unit, shape => 'house', color=> 'red', label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor );
		$graph_buildings_units-> add_node($unit, shape => 'house', color=> 'red', label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor );
		$graph_buildings      -> add_node($unit, shape => 'house', color=> 'red', label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor );
	}
	elsif ( $upgrade ) {
		$graph_all             -> add_node($unit, shape => 'parallelogram', color=> 'green' , label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor);
		##$graph_buildings_units -> add_node($unit, shape => 'parallelogram', color=> 'green' , label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor);
		##$graph_buildings       -> add_node($unit, shape => 'parallelogram', color=> 'green' , label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor);
	}
	# combat or work unit
	else {
		$graph_all             -> add_node($unit, color => 'magenta', label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor);
		$graph_buildings_units -> add_node($unit, color => 'magenta', label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor);
		##$graph_buildings       -> add_node($unit, color => 'magenta', label => $unit_pretty, URL => $url, fontcolor => $svg_fontcolor);
	}

}

sub g_add_edge {
	my ( $faction, $unit_from, $unit_to, $style, $relation ) = @_;

	push @{ $c_edges{"$faction"} },                 "$unit_from:$unit_to:$style:$relation";
	push @{ $c_edges_from{"$faction:$unit_from"} }, "$unit_from:$unit_to:$style:$relation";

	$edges_relation{"$faction:$unit_from:$unit_to:$relation"}=1;
}


sub check_multi_hops {

	my ( $faction ) = @_;

	# loop over all edges and check each
	foreach my $edge ( @{$c_edges{"$faction"}} ) {
		
		my ( $unit_from, $unit_to, $style, $relation ) = split(/:/, $edge );

		print "#################edge_check1: $unit_from -> $unit_to\n";

		# only check for Requirement and Upgrade-Requirement
		if ( $relation =~ /Requirement/i ) {
			# do recursion in check_edges:
			if ( &check_edges( $faction, $unit_from, $unit_to, 0 ) ) {
				$multi_hop{"$faction:$unit_from:$unit_to"}=1;
				print "edge_check5: $faction:$unit_from:$unit_to =1\n";
			}
		}
		# find multihop from building via upgrade to building to include in graphs without upgrades
		# f.e. techndrome via advanced architecture to defense tower
		elsif ( 
			$relation eq "Upgrade" && 
			$building{"$faction:$unit_from"}
		) {
			# in our example, check all edges from advanced architecture for links to buildings
			REC: foreach my $edge ( @{$c_edges_from{"$faction:$unit_to"}} ) {
				my ( $o_unit_from, $o_unit_to, $o_style, $o_relation ) = split(/:/, $edge );
				if ( $building{ "$faction:$o_unit_to"} ) {
					print "upgrade-multihop: $unit_from to $o_unit_to\n";
					$multi_hop_upgrade{"$faction:$unit_from:$o_unit_to"}=1;
				}
			}
				
		}
	}
}

sub check_edges {

	# recursive function to walk thru the branch of the tree and find a multi_hop

	my ( $faction, $unit_from, $unit_to, $recursion_depth ) = @_;

	$recursion_depth++;

	my $found_multihop=0;

	print "edge_check2: $faction, $unit_from, $unit_to, $recursion_depth\n";

	REC: foreach my $edge ( @{$c_edges_from{"$faction:$unit_from"}} ) {
		my ( $o_unit_from, $o_unit_to, $o_style, $o_relation ) = split(/:/, $edge );

		print "edge_check3: $o_unit_from -> $o_unit_to\n";
		if ( 
			$unit_to eq $o_unit_to     && 
			# skip the record we're just checking
			$recursion_depth > 1
		) {
			$found_multihop=1;
			print "edge_check4: found an end at $o_unit_from to $unit_to\n";
			last REC;
		}
		else {
			# here &check_edges() recursivly calls itself until it finds something or $recursion_depth ==4:
			if ( $recursion_depth < 4 ) {
				$found_multihop = &check_edges( $faction, $o_unit_to, $unit_to, $recursion_depth );
			}
		}
		last REC if ( $found_multihop );
	}
	return $found_multihop; 
}


sub export_graphs {

	my ( $faction ) = @_;

	my ( @as_functions ) = split(/;/, $cfg->val('all', 'export_graph'));

	foreach my $as_function ( @as_functions ) {

		my $extension = $as_function;
		if ( $extension =~ s/^as_// ) {

				# all
				my $graph_file ="$out_path/$images_path/$faction/${faction}_techtree_graph_all.$extension";
				$graph_files{ "$faction:$extension:all" } = $graph_file;
				open (OUTGRAPH, "> $graph_file") || die "cant write graph-file: $graph_file\n";
				print OUTGRAPH $graph_all->$as_function;
				close OUTGRAPH;

				# buildings & units
				$graph_file ="$out_path/$images_path/$faction/${faction}_techtree_graph_buildings.$extension";
				$graph_files{ "$faction:$extension:buildings" } = $graph_file;
				open (OUTGRAPH, "> $graph_file") || die "cant write graph-file: $graph_file\n";
				print OUTGRAPH $graph_buildings->$as_function;
				close OUTGRAPH;

				# buildings
				$graph_file ="$out_path/$images_path/$faction/${faction}_techtree_graph_buildings_units.$extension";
				$graph_files{ "$faction:$extension:buildings_units" } = $graph_file;
				open (OUTGRAPH, "> $graph_file") || die "cant write graph-file: $graph_file\n";
				print OUTGRAPH $graph_buildings_units->$as_function;
				close OUTGRAPH;
		}
		else {
			die "ERROR: wrong function in ini-files: export_graph: $as_function\n\n";
		}

	}
}

sub build_clickable_map {
	my ( $faction ) = @_;

	my %cmapx;
	$cmapx{'all'}             = $graph_all->as_cmapx;
	$cmapx{'buildings'}       = $graph_buildings->as_cmapx;
	$cmapx{'buildings_units'} = $graph_buildings_units->as_cmapx;

	foreach my $map ('all', 'buildings', 'buildings_units') {

		my $png   = $graph_files{"$faction:png:map"};

		my $graph_file = "$out_path/${faction}_techtree_clickable_map_${map}.html";
		$graph_files{ "$faction:click_map:${map}" } = $graph_file;

		open (OUTHTML, "> $graph_file") || die "cant write graph-file: $graph_file\n";

		push @all_html_pages, $graph_file;

		print OUTHTML &header("Clickable Techtree Diagram for $faction");

		print OUTHTML "<img src=\"images/$faction/${faction}_techtree_graph_${map}.png\" usemap=#test>\n";
		print OUTHTML $cmapx{$map};

		print OUTHTML $map_legend;
		close OUTHTML;
	}

}

sub calc_cost_of_unit {

	# calculate amount for morphed units, f.e. battemachine morphed from technician
	# technician costs 150 gold
	# battlemachine costs 150 gold, 300 wood with 40% discount = 90 gold + 180 wood
	# = 240 gold + 180 wood total (food = 1 from battlemachine because technician is gone)
	# note: the "discount" is actually a refund, you pay 150 gold at morphing to the battlemachine and get 60 back when it's done

	my ( $u, $created_by_u, $method, $discount, $ignore_food ) = @_;

	print "calc_cost_of_unit: $u\n";
	my ( $faction, $unit ) = split(/:/, $u );

	my ( $cost, $cost_icon, $cost_icon_overview );
	my ( $morph_cost, $morph_cost_icon, $morph_cost_icon_overview );

	my $cost_icon_discount;

	my %morph_cost_resource;

	if ( $method eq "morph" ) {
		foreach my $created_by_unit_tmp ( @{$created_by_unit{ "$faction:$created_by_u" }} ) {

			my ( $o_created_by_u, $o_method, $o_discount ) = split(/:/, $created_by_unit_tmp );

			( $morph_cost, $morph_cost_icon, $morph_cost_icon_overview ) = &calc_cost_of_unit("$faction:$created_by_u", $o_created_by_u, $o_method, $o_discount, "ignore_food" );

			my (@res_amounts) = split(/,/, $morph_cost );
			foreach my $res_amount ( @res_amounts ) {
				my ( $res, $amount ) = split(/:/, $res_amount );
				$morph_cost_resource{ $res } = $amount;
				print "morph_cost_resource of $u res: $res = $amount\n";
			}
		}
	}


	# loop over @resource_order to get it in the proper order (medival programmig ;)
	foreach my $resource_now ( @resource_order ) {
		foreach my $resource_requirement ( reverse sort @{$c_resource_requirement{$u}} ) {
			my ( $resource, $amount ) = split(/:/, $resource_requirement );

			# ignore food, housing, energy if i have to calculate the cost of a unit morphed from 
			if ( 
				!$ignore_food ||
				$ignore_food && (
				#$method ne "morph" ||
				#$method eq "morph" && (
					$resource eq "gold" || 
					$resource eq "wood" || 
					$resource eq "stone" 
					)
			) {

				my ($amount_with_discount, $amount_total);

				if ( $resource eq $resource_now ) {
					# oly use 1st letter in uppercase
					my $resource_char = $resource;
					$resource_char =~ s/^(.).+/\u$1/;

					if ( $method && $method eq "morph" ) {
						# get discount/refund and add cost of unit morphed from
						if ( $resource eq "gold" || $resource eq "wood" || $resource eq "stone" ) {
							$amount_with_discount = int($amount * (100-$discount)/100);
						}
						else {
							# no discount for food, energy, housing, ...
							$amount_with_discount = $amount;
						}

						$amount_total = $amount_with_discount + ($morph_cost_resource{ $resource } || 0 );
						$cost_icon_discount .= &html_icon_resource($resource, 16)."&nbsp;$amount_with_discount&nbsp;";
					}
					else {
						$amount_total = $amount;
					}

					print "amount_total: $amount_total\n";
					$cost      .= "$resource:$amount_total,";

					my $add_nbsp = "";
					if ( $amount_total < 100 && ( $resource eq "gold" || $resource eq "wood" || $resource eq "stone" ) ) {
						$add_nbsp = "&nbsp;&nbsp;";
					}

					$cost_icon          .= &html_icon_resource($resource, 16)."&nbsp;$amount_total&nbsp;";
					$cost_icon_overview .= &html_icon_resource($resource, 16)."&nbsp;$add_nbsp$amount_total&nbsp;";
				}
			}
		}
	}
	# remove trailing ","
	chop $cost;
	print "cost: $cost\n";

	my $cost_icon_total;

	if ( $method && $method eq "morph" ) {
		$cost_icon_total = "$cost_icon <BR>(Cost for ".&format_name($unit);
		if ( $discount ) {
			$cost_icon_total .= " with $discount&nbsp;% discount = $cost_icon_discount";
		}
		else {
			$cost_icon_total .= " = $cost_icon_discount";
		}
		$cost_icon_total .= "<BR>&nbsp;+ cost for ".&link_unit($created_by_u)." = $morph_cost_icon )";
	}
	else {
		$cost_icon_total= $cost_icon;
	}
	return ( $cost, $cost_icon_total, $cost_icon_overview );
}



sub html_icon {
	my ( $u, $size ) = @_;
	my ( $faction, $unit ) = split(/:/, $u );

	return "<IMG SRC=\"$images_path/$faction/".$icon{$u}."\" HEIGHT=$size WIDTH=$size>";
}

sub html_icon_command {
	my ( $c, $size ) = @_;
	my ( $faction, $unit, $command ) = split(/:/, $c );

			#$command_icon{ $c } = get_value($xpath, "/unit/commands/command/name[\@value='$command']/parent::*/image/\@path");
			#if ( $command_icon{ $c }) {
				#print "command icon: ".$command_icon{ $c }." for $c\n" ;
				## commented to save time, not used anyway yet
				#system "cp $faction_path/$units_path/$unit/$command_icon{ $c } $out_path/$images_path/${faction}";


	if ( !$command_icon{$c} ) {
		print "WARNING: no command icon for $c\n";
	}
	return "<IMG SRC=\"$images_path/$faction/".$command_icon{$c}."\" HEIGHT=$size WIDTH=$size>";
}

sub link_unit {

	# create HTML-link to a unit
	my ($unit, $link_text ) = @_;

	# default to pretty unit name if no link_text passed
	if ( !$link_text ) {
		$link_text = &format_name($unit);
	}

	my $link = "<A HREF=\"#${unit}_full\">$link_text</A>";
	return $link;
}

sub link_attack_and_armor_types {
	my ( $link_to ) = @_;
	return "<a href=\"attack_and_armor_types.html#$link_to\">".&format_name($link_to)."</A>";
}

sub show_attack {

	my ( $c, $max_strength_vs_land, $max_strength_vs_land_var, $max_strength_vs_air, $max_strength_vs_air_var, $max_range, $max_move_speed, $skill_on_hold_position ) = @_;
	my ( $faction, $unit, $command ) = split(/:/, $c );
	my $command_pretty = &format_name( $command );

	my $full_attack_tmp = "<TR><TD>Attack Command: $command_pretty".&html_icon_command( $c, 32 )."<TD>\n";

	my $skill = $attack_skill{ $c };
	# attacks have own move_skills (charge)
	my $move_skill = $move_skill{ $c };
	my $s = "$faction:$unit:$skill";
	my $ms = "$faction:$unit:$move_skill" if ( $move_skill);

	my $skill_short = $skill;
	$skill_short =~ s/_skill$//;
	my $skill_pretty = &format_name( $skill_short );

	if ( $attack_land{ $s } ) {

		if ( $attack_strenght{ $s } > $max_strength_vs_land ) {
			$max_strength_vs_land     = $attack_strenght{ $s };
			$max_strength_vs_land_var = $attack_var{ $s };
		}
	}
	if ( $attack_air{ $s } ) {

		if ( $attack_strenght{ $s } > $max_strength_vs_air ) {
			$max_strength_vs_air     = $attack_strenght{ $s };
			$max_strength_vs_air_var = $attack_var{ $s };
		}
	}

	if ( 
		$attack_land{ $s } &&
		$attack_air{ $s }
	) {
		$full_attack_tmp .= "Target: land and air units<BR>\n";
	}
	elsif ( 
		$attack_air{ $s }
	) {
		$full_attack_tmp .= "Target: only air units<BR>\n";
	}
	elsif ( 
		$attack_land{ $s } 
	) {
		$full_attack_tmp .= "Target: only land units<BR>\n";
	}
	

	if ( $attack_range{ $s } > $max_range ) {
		$max_range = $attack_range{ $s };
	}


	$full_attack_tmp .= "Strength: $attack_strenght{ $s }+-$attack_var{ $s }<BR>\n";

	# show attack as range ( 100-200)
	#$full_attack_tmp .= "Strength: ".($attack_strenght{ $s }-$attack_var{ $s })."-".($attack_strenght{ $s }+$attack_var{ $s })."<BR>\n";

	#$full_attack_tmp .= "Strength-Variance: ".$attack_var{ $s }."<BR>\n";
	#$full_attack_tmp .= "Strength: ".$attack_strenght{ $s }."<BR>\n";
	#$full_attack_tmp .= "Strength-Variance: ".$attack_var{ $s }."<BR>\n";

	$full_attack_tmp .= "Range: ".$attack_range{ $s }."<BR>\n";
	if ( $splash{ $s } ) {
		$full_attack_tmp .= "Splash-Radius: $splash_radius{ $s }<BR>\n";
		$full_attack_tmp .= "Splash also damages own units!<BR>\n" if ($splash_damage_all{ $s });
	}
	$full_attack_tmp .= "Type: ".&link_attack_and_armor_types($attack_type{ $s })." <BR>\n";
	$full_attack_tmp .= "Attack Speed: ".$speed{ $s }."<BR>\n";
	$full_attack_tmp .= "Start Time: ".$attack_start_time{ $s }."<BR>\n";
	$full_attack_tmp .= "Energy-Cost:".$ep_cost{ $s }."<BR>\n" if ( $ep_cost{ $s });
	if ( $skill_on_hold_position ) {
		my ( $s, $requirement ) = split(/;/, $skill_on_hold_position );
		if ( $skill eq $s ) {
			$full_attack_tmp .= "This Attack Skill is used on \"Hold Position\"<BR>\n";
			$full_attack_tmp .= "(\"Hold Position\" requires ".&link_unit( $requirement).")<BR>\n" if ( $requirement);
		}
	}

	if ( $ms) {
		print "speed: $speed{ $ms } of $ms\n" ;
		if ( $speed{ $ms } > $max_move_speed ) {
			$max_move_speed = $speed{ $ms };
			$full_attack_tmp .= "Special Move Speed: ".$speed{ $ms }." (".&format_name( $move_skill).")<BR>\n";
		}
	}

	return ( $full_attack_tmp, $max_strength_vs_land, $max_strength_vs_land_var, $max_strength_vs_air, $max_strength_vs_air_var, $max_range, $max_move_speed );
}

sub html_icon_resource {
	# return <IMG SRC=...> for resource (gold, wood, stone, ...)
	my ($resource, $size) = @_;
	return "<IMG SRC=\"images/resources/$resource.bmp\" HEIGHT=$size WIDTH=$size>";
}

sub load_tip_files {

	foreach my $faction_path ( @factions ) {
		my $faction = $faction_path;
		$faction =~ s/.+\///;
		my $faction_pretty = &format_name( $faction );

		my $tip_file = "$faction_path/lang/${faction}_en.lng";
		print "tip-file: $tip_file\n";

		open (TIPS, "< $tip_file") || die "can't read tip-file: $tip_file\n";
		while (my $line =<TIPS>) {
			next if ( $line =~ /^;/ );
			if ( $line =~ /(.+)=(.+)/ ) {
				my $name = $1;
				my $key  = $2;

				$tips{ $name } = $key;

			}
		}
		close TIPS;
	}
	
}
