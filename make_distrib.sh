###############################################################################
#                                                                             #
# make_distrib.sh - Makes an HLmaps distribution package                      #
#       - Copyright Scott McCrory as part of the HLmaps distribution          #
#       - Distributed under the GNU Public License - see docs for more info   #
#       - http://hlmaps.sourceforge.net                                       #
#                                                                             #
###############################################################################
# CVS $Id$
###############################################################################

# First refresh the docs in the HTML tree
chown -R smccrory.smccrory *
rm -rf ../hlmaps_php-release/*
cp -f CHANGELOG CONTRIBUTING CONTRIBUTORS hlmaps.conf.distrib hlmaps.cron hlmaps.inc hlmaps.php INSTALL INSTALL.MySQL LICENSE make_distrib.sh README styles STYLES101 TODO ../hlmaps_php-release/

# Tar and gzip the release and html packages
cd .. 
tar cvf hlmaps_php-release.tar hlmaps_php-release
gzip -9 hlmaps_php-release.tar
