#
# spec file for package libzypp-zmd-backend (Version 7.1.1.0)
#
# Copyright (c) 2006 SUSE LINUX Products GmbH, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

# norootforbuild

Name:           libzypp-zmd-backend
BuildRequires:  dejagnu gcc-c++ libzypp-devel sqlite-devel
License:        GPL
Group:          System/Management
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Autoreqprov:    on
Requires:       libzypp >= %( echo `rpm -q --queryformat '%{VERSION}-%{RELEASE}' libzypp`)
Provides:       zmd-backend
Provides:       zmd-librc-backend
Obsoletes:      zmd-librc-backend
Summary:        ZMD backend for Package, Patch, Pattern, and Product Management
Version:        7.1.1.0
Release:        48
Source:         zmd-backend-%{version}.tar.bz2
Prefix:         /usr

%description
This package provides binaries which enable the ZENworks daemon (ZMD)
to perform its management tasks on the system.



Authors:
--------
    Klaus Kaempf <kkaempf@suse.de>

%prep
%setup -q -n zmd-backend-7.1.1.0

%build
mv configure.ac x
grep -v devel/ x > configure.ac
autoreconf --force --install --symlink --verbose
%{?suse_update_config:%{suse_update_config -f}}
./configure --prefix=%{prefix} --libdir=%{_libdir} --mandir=%{_mandir} --disable-static
make %{?jobs:-j %jobs}
#make check

%install
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
install -m 644 logrotate_zmd_backend  $RPM_BUILD_ROOT/etc/logrotate.d/zmd-backend
# Create filelist with translatins
#%{find_lang} zypp

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/libzmd-backend.*
%dir %{_libdir}/zmd
%{_libdir}/zmd/*
%dir %{prefix}/include/zmd-backend
%{prefix}/include/zmd-backend/*
/etc/logrotate.d/zmd-backend

%changelog -n libzypp-zmd-backend
* Tue May 30 2006 - kkaempf@suse.de
- re-enable callback (r3398, r3425) based on environment variable
  "KEYRING_CALLBACK" set by ZMD (#173920)
- output error heading to stdout instead of stderr (#177746)
- rev 3453
* Tue May 23 2006 - mvidner@suse.cz
- Disabled callbacks (r3398) because zmd is not ready yet.
- rev 3425
* Tue May 23 2006 - mvidner@suse.cz
- If source is not found by alias, try URL (#177543).
- rev 3422
* Thu May 18 2006 - hmueller@suse.de
- changes id strings from
  1 to 7 and 21 to 27 ( #173920)
* Thu May 18 2006 - kkaempf@suse.de
- dont keep sources list in query-system, its unused. (#176301)
- rev 3409
* Thu May 18 2006 - kkaempf@suse.de
- implement key/digest callbacks (#173920)
- rev 3398
* Wed May 17 2006 - kkaempf@suse.de
- adapt to and require libzypp >= 1.0.0
- rev 3391
* Tue May 16 2006 - kkaempf@suse.de
- fix large file support detection.
- treat '@local' as zmd owned source (#170246)
- rev 3380
* Mon May 15 2006 - kkaempf@suse.de
- match large file support to zypp package.
- rev 3377
* Fri May 05 2006 - kkaempf@suse.de
- set package_filename and package_url right even for zmd internal
  services (#171376)
- rev 3346
* Fri Apr 28 2006 - kkaempf@suse.de
- create language resolvables (#170313)
- rev 3265
* Thu Apr 27 2006 - kkaempf@suse.de
- honor YaST lock file (#170113)
- rev 3254
* Thu Apr 27 2006 - kkaempf@suse.de
- dont exit early if url already known (#170113)
- rev 3252
* Wed Apr 26 2006 - kkaempf@suse.de
- enable target store in 'transact' helper.
- show the message content, not the resolvable.
- rev 3247
* Wed Apr 26 2006 - kkaempf@suse.de
- Fix rev 3218, add missing parameter (#168060)
- rev 3237
* Tue Apr 25 2006 - kkaempf@suse.de
- delete committed transactions from database (#168931)
- rev 3229
* Tue Apr 25 2006 - kkaempf@suse.de
- adapt KeyRing api to libzypp (#168060)
- rev 3218
* Mon Apr 24 2006 - kkaempf@suse.de
- get source syncing right (#167958, #168051, #168652)
- rev 3211
* Sun Apr 23 2006 - kkaempf@suse.de
- sync Message and Script objects to zmd database (#168607)
- rev 3183
* Thu Apr 20 2006 - kkaempf@suse.de
- add 'alias=' parameter with "&" if url already contains "?"
  (#168030)
- rev 3161
* Thu Apr 20 2006 - kkaempf@suse.de
- Don't mis-interpret commit results on package removals (#167822)
- Instantiate KeyRing and Digest callbacks immediately after ZYpp.
- rev 3158
* Wed Apr 19 2006 - mvidner@suse.cz
- service-delete: Only add alias to URL for ZYPP type sources (#160613).
  Now YUM sources deleted by rug should get deleted from YaST too.
- rev 3148
* Wed Apr 19 2006 - kkaempf@suse.de
- Instantiate KeyRing and Digest callbacks in each helper (#167245)
- rev 3145
* Tue Apr 18 2006 - kkaempf@suse.de
- bugfix 165845
  parse-metadata: the source type determines the owner, not the url
  transact: use real zypp source if package is not provided by zmd
- rev 3131
* Thu Apr 13 2006 - kkaempf@suse.de
- add KeyRing callbacks to transact and parse-metadata
- handle dry_run and nosignature on commit
- dont fail on dry_run (#164583)
- rev 3092
* Wed Apr 12 2006 - kkaempf@suse.de
- write all atoms to support multi-arch patch deps (#165556)
- rev 3077
* Wed Apr 12 2006 - kkaempf@suse.de
- create zypp source when added via zmd (#165103)
- rev 3072
* Mon Apr 10 2006 - kkaempf@suse.de
- honor patch messages (#160015)
- rev 3040
* Mon Apr 10 2006 - kkaempf@suse.de
- query-status and parse-metadata don't need a read-write lock.
- rev 3034
* Fri Apr 07 2006 - kkaempf@suse.de
- honor 'uri' argument to parse-metadata (#160402, finally)
- report vendor as "suse" (#164422)
- rev 3000
* Thu Apr 06 2006 - kkaempf@suse.de
- allow more arguments to parse-metadata
- rev 2982
* Wed Apr 05 2006 - kkaempf@suse.de
- fix url for local packages.
- rev 2963
* Wed Apr 05 2006 - kkaempf@suse.de
- finally set package_url/package_filename right.
- rev 2943
* Wed Apr 05 2006 - kkaempf@suse.de
- change version to 7.1.1.0 so it matches zmd.
- log exit code for all helpers.
- check url parameter to parse-metadata.
- rev 2930
* Tue Apr 04 2006 - kkaempf@suse.de
- add 'service-delete' helper.
- rev 2912
* Mon Apr 03 2006 - kkaempf@suse.de
- plainRpm() is deprecated, use location()
- rev 2893
* Sun Apr 02 2006 - kkaempf@suse.de
- add force_remote flag to handle the case where zmd does the
  download, so metadata appears local to zypp, but the real
  source is remote (#160402).
- rev 2882
* Sun Apr 02 2006 - kkaempf@suse.de
- create dummy source for local packages (#147765)
- rev 2881
* Thu Mar 30 2006 - kkaempf@suse.de
- fix 'update-status', the 'status' attribute was moved to the
  resolvables table
- rev 2815
* Thu Mar 30 2006 - kkaempf@suse.de
- let the target fill the pool.
- rev 2792.
* Thu Mar 30 2006 - kkaempf@suse.de
- add resolvable kind to output of query-pool.
- move the zsources table to separate database (#161318)
- rev 2786
* Thu Mar 30 2006 - kkaempf@suse.de
- prepare for "query-pool" helper.
- treat "@system" catalog as subscribed (#161699)
- rev 2779
* Wed Mar 29 2006 - kkaempf@suse.de
- always treat zmd sources as 'remote' (#160402)
- rev 2757
* Wed Mar 29 2006 - kkaempf@suse.de
- report proper error if .rpm file can't be found (#160402)
- dont ask for media 0
- rev 2751
* Wed Mar 29 2006 - kkaempf@suse.de
- call the correct 'getZYpp()' function.
- fix argument parsing in resolve-dependencies (#161699)
- rev 2748
* Wed Mar 29 2006 - kkaempf@suse.de
- adapt to catalogs table change in zmd, now passes 'subscribed'
  (#161395)
- rev 2734, needs zmd >= rev 26297
* Mon Mar 27 2006 - visnov@suse.cz
- try to use source alias for YaST source
- rev 2697
* Fri Mar 24 2006 - kkaempf@suse.de
- Handle zypp global lock (#160319)
- rev 2674
* Fri Mar 24 2006 - kkaempf@suse.de
- fix sql query (#160221)
- rev 2668
* Fri Mar 24 2006 - kkaempf@suse.de
- pass summary and description to zmd (#160510)
- rev 2651
* Thu Mar 23 2006 - mvidner@suse.cz
- Pass the source data also to zypp, if it does not know it yet
  (#156139).
- rev 2644
* Thu Mar 23 2006 - kkaempf@suse.de
- return 0 if no transactions are pending (#158179)
- rev 2636
* Wed Mar 22 2006 - kkaempf@suse.de
- adapt to new table layout. status and category are generic
  attributes now (#159936)
- rev 2619
* Sun Mar 19 2006 - kkaempf@suse.de
- save zmd table id in atom resolvable.
- compute resolvable status (establishPool) before resolving
  dependencies.
- rev 2564
* Sun Mar 19 2006 - kkaempf@suse.de
- provide sqlite table access as libzmd-backend.
- rev 2552
* Thu Mar 16 2006 - kkaempf@suse.de
- fix sqlite queries (#157457)
- rev 2508
* Thu Mar 16 2006 - kkaempf@suse.de
- query-file: default to "@local" catalog if not specified
  otherwise (#157405).
- consider "file://" as local (#157405).
- rev 2499
* Thu Mar 16 2006 - kkaempf@suse.de
- add patterns and products to the pool.
- set the resolver to non-interactive (#158062).
- rev 2492
* Tue Mar 14 2006 - kkaempf@suse.de
- dependency errors go to stdout instead of stderr.
- rev 2458
* Tue Mar 14 2006 - kkaempf@suse.de
- insert packages and patches to pool (#157684).
- dont collect error lines but output them directly.
- rev 2457
* Mon Mar 13 2006 - kkaempf@suse.de
- copy resolver errors to stderr (#157495, #157324)
- rev 2451
* Sun Mar 12 2006 - kkaempf@suse.de
- honor 'kind' attribute in resolvables table (#157497)
- properly read patches from sqlite tables.
- rev 2432
* Sun Mar 12 2006 - kkaempf@suse.de
- improve transact report messaging (#157337, partly)
- workaround for #157469
- rev 2427
* Sat Mar 11 2006 - kkaempf@suse.de
- Implement 'update-status' helper (#156420)
- rev 2419
* Fri Mar 10 2006 - kkaempf@suse.de
- Fill package_url attribute (#156742)
- rev 2406 (needs libzypp revision >= 2406)
* Thu Mar 09 2006 - kkaempf@suse.de
- Implemented CD change callback.
- rev 2377
* Wed Mar 08 2006 - kkaempf@suse.de
- only write package_filename if zypp knows how to get the package
  (cd, dvd, nfs, smb sources). Else leave it empty and let ZMD
  do the download. (#156076)
- rev 2362
* Wed Mar 08 2006 - kkaempf@suse.de
- use given, not internal catalog name.
- rev 2360
* Tue Mar 07 2006 - kkaempf@suse.de
- split of from libzypp main package.
- bump version to 7.1.1 so its in sync with zmd.
- add logrotate config.
