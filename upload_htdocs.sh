###############################################################################
#                                                                             #
# upload_htdocs.sh - Uploads the HLmaps_php htdocs package to SourceForge     #
#       - Copyright Scott McCrory as part of the HLmaps_php distribution      #
#       - Distributed under the GNU Public License - see docs for more info   #
#       - http://hlmaps.sourceforge.net                                       #
#                                                                             #
###############################################################################
# CVS $Id$
###############################################################################

cd ..

# Copy the updated web site docs to SourceForge
scp hlmaps_php-htdocs.tar.gz smccrory@hlmaps.sourceforge.net:/home/groups/h/hl/hlmaps/

# Open a shell session to allow us to decompress it
ssh -l smccrory hlmaps.sourceforge.net

