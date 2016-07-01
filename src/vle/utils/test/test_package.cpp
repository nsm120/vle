/*
 * This file is part of VLE, a framework for multi-modeling, simulation
 * and analysis of complex dynamical systems.
 * http://www.vle-project.org
 *
 * Copyright (c) 2003-2016 Gauthier Quesnel <quesnel@users.sourceforge.net>
 * Copyright (c) 2003-2016 ULCO http://www.univ-littoral.fr
 * Copyright (c) 2007-2016 INRA http://www.inra.fr
 *
 * See the AUTHORS or Authors.txt file for copyright owners and
 * contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE utils_library_test
#include <boost/test/included/unit_test.hpp>
#include <boost/version.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/config.hpp>
#include <boost/assign.hpp>
#include <stdexcept>
#include <limits>
#include <fstream>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <string>
#include <iterator>
#include <iostream>
#include <sstream>
#include <numeric>
#include <memory>
#include <vle/utils/i18n.hpp>
#include <vle/utils/Algo.hpp>
#include <vle/utils/DateTime.hpp>
#include <vle/utils/Package.hpp>
#include <vle/utils/Context.hpp>
#include <vle/utils/Rand.hpp>
#include <vle/utils/Tools.hpp>
#include <vle/utils/RemoteManager.hpp>
#include <vle/utils/Filesystem.hpp>
#include <vle/vle.hpp>

using namespace vle;

struct F
{
    vle::Init a;
    vle::utils::Path current_path;

    F()
    {
        current_path = vle::utils::Path::temp_directory_path();
        current_path /= vle::utils::Path::unique_path("vle-%%%%-%%%%-%%%%");
        current_path.create_directory();

        /* We need to ensure each file really installed. */
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        if (not current_path.is_directory())
            throw std::runtime_error("Fails to found temporary directory");

#ifdef _WIN32
        ::_putenv((vle::fmt("VLE_HOME=%1%")
                   % current_path.string()).str().c_str());
#else
        ::setenv("VLE_HOME", current_path.string().c_str(), 1);
#endif

        vle::utils::Path::current_path(current_path);
        std::cout << "test start in " << current_path.string() << '\n';

        auto ctx = vle::utils::make_context();
        ctx->write_settings();
    }

    ~F()
    {
        std::cout << "test finish in " << current_path.string() << '\n';
    }
};

BOOST_GLOBAL_FIXTURE(F);

BOOST_AUTO_TEST_CASE(show_package)
{
    auto ctx = vle::utils::make_context();
    using vle::utils::PathList;
    using vle::utils::Package;

    Package pkg(ctx, "show_package");
    pkg.create();
    pkg.wait(std::cerr, std::cerr);

    /* We need to ensure each file really installed. */
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    pkg.configure();
    pkg.wait(std::cerr, std::cerr);
    pkg.build();
    pkg.wait(std::cerr, std::cerr);
    pkg.install();
    pkg.wait(std::cerr, std::cerr);

    std::cout << "getBinaryPackagesDir:\n";
    {
        auto paths = ctx->getBinaryPackagesDir();
        for (std::size_t i = 0, e = paths.size(); i != e; ++i)
            std::cout << i << ": " << paths[i].string() << '\n';
    }

    std::cout << "getBinaryPackages:\n";
    {
        auto paths = ctx->getBinaryPackages();
        for (std::size_t i = 0, e = paths.size(); i != e; ++i)
            std::cout << i << ": " << paths[i].string() << '\n';
    }

    // 2 binary packages: the show_package build previously and the
    // vle.output package provided with VLE.
    BOOST_REQUIRE(ctx->getBinaryPackages().size() == 2);
    vle::utils::Path p = pkg.getExpDir(vle::utils::PKG_BINARY);

    std::cout << "\ngetExpDir           : " << p.string()
              << "\ngetExperiments      :";
    auto vpz = pkg.getExperiments();

    for (const auto& elem : vpz)
        std::cout << elem.string() << ' ';

    BOOST_REQUIRE(vpz.size() == 1);


    auto modules = ctx->get_dynamic_libraries(
        "show_package",
        vle::utils::Context::ModuleType::MODULE_DYNAMICS);

    BOOST_REQUIRE(modules.size() == 1);
}

BOOST_AUTO_TEST_CASE(remote_package_check_package_tmp)
{
    auto ctx = vle::utils::make_context();

    utils::Package pkg_tmp(ctx, "remote_package_check_package_tmp");
    pkg_tmp.create();
    pkg_tmp.wait(std::cerr, std::cerr);
    pkg_tmp.configure();
    pkg_tmp.wait(std::cerr, std::cerr);
    pkg_tmp.build();
    pkg_tmp.wait(std::cerr, std::cerr);
    pkg_tmp.install();
    pkg_tmp.wait(std::cerr, std::cerr);

    /* We need to ensure each file really installed. */
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    utils::RemoteManager rmt(ctx);
    utils::Packages results;

    {
        rmt.start(utils::REMOTE_MANAGER_SEARCH, ".*", nullptr);
        rmt.join();
        rmt.getResult(&results);
        BOOST_REQUIRE_EQUAL(results.empty(), true);
        BOOST_REQUIRE_EQUAL(results.size(), 0u);
    }

    {
        rmt.start(utils::REMOTE_MANAGER_LOCAL_SEARCH, ".*", nullptr);
        rmt.join();
        rmt.getResult(&results);

        //
        // results.size() == 2, remote_manager_local_search and
        // show_package (description.txt are the same, name is MyProject.
        //

        BOOST_REQUIRE(results.empty() == false);
        BOOST_REQUIRE(results.size() == 2);
        BOOST_REQUIRE(results[0].name == "MyProject" and
                      results[1].name == "MyProject");
    }
}

BOOST_AUTO_TEST_CASE(remote_package_local_remote)
{
    auto ctx = vle::utils::make_context();
    utils::PackageId pkg;

    pkg.size = 0;
    pkg.name = "name";
    pkg.distribution = "distribution";
    pkg.maintainer = "me";
    pkg.description = "too good";
    pkg.url = "http://www.vle-project.org";
    pkg.md5sum = "1234567890987654321";
    pkg.tags = { "a", "b", "c" };

    {
        utils::PackageLinkId dep = { "a", 1, 1, 1,
                                     utils::PACKAGE_OPERATOR_EQUAL };
        pkg.depends = utils::PackagesLinkId(1, dep);
    }

    {
        utils::PackageLinkId dep = { "a", 1, 1, 1,
                                     utils::PACKAGE_OPERATOR_EQUAL };
        pkg.builddepends = utils::PackagesLinkId(1, dep);
    }

    {
        utils::PackageLinkId dep = { "a", 1, 1, 1,
                                     utils::PACKAGE_OPERATOR_EQUAL };
        pkg.conflicts = utils::PackagesLinkId(1, dep);
    }

    pkg.major = 1;
    pkg.minor = 2;
    pkg.patch = 3;

    vle::utils::Path path = ctx->getRemotePackageFilename();

    {
        std::ofstream ofs(path.string());
        BOOST_REQUIRE(ofs.is_open());
        ofs << pkg << "\n";
        BOOST_REQUIRE(ofs.good());
    }

    /* We need to ensure each file really installed. */
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    utils::RemoteManager rmt(ctx);

    {
        utils::Packages results;
        rmt.start(utils::REMOTE_MANAGER_SEARCH, ".*", nullptr);
        rmt.join();
        rmt.getResult(&results);
        BOOST_REQUIRE_EQUAL(results.empty(), false);
        BOOST_REQUIRE_EQUAL(results.size(), 1u);

        BOOST_REQUIRE(results[0].name == "name");
    }

    {
        utils::Packages results;
        rmt.start(utils::REMOTE_MANAGER_LOCAL_SEARCH, ".*", nullptr);
        rmt.join();
        rmt.getResult(&results);

        for (const auto& elem : results)
            std::cout << elem;

        BOOST_REQUIRE_EQUAL(results.empty(), false);
        BOOST_REQUIRE_EQUAL(results.size(), 2u);
    }
}

BOOST_AUTO_TEST_CASE(remote_package_read_write)
{
    auto ctx = vle::utils::make_context();

    {
        std::ofstream OX("/tmp/X.dat");
        std::ofstream ofs(ctx->getRemotePackageFilename().string());

        for (int i = 0; i < 10; ++i) {
            utils::PackageId pkg;

            pkg.size = i;
            pkg.name = (fmt("name-%1%") % i).str();
            pkg.distribution = "distribution";
            pkg.maintainer = "me";
            pkg.description = "too good";
            pkg.url = "http://www.vle-project.org";
            pkg.md5sum = "1234567890987654321";
            pkg.tags = { "a", "b", "c" };

            {
                utils::PackageLinkId dep = { "a", 1, 1, 1,
                                             utils::PACKAGE_OPERATOR_EQUAL };
                pkg.depends = utils::PackagesLinkId(1, dep);
            }

            {
                utils::PackageLinkId dep = { "a", 1, 1, 1,
                                             utils::PACKAGE_OPERATOR_EQUAL };
                pkg.builddepends = utils::PackagesLinkId(1, dep);
            }

            {
                utils::PackageLinkId dep = { "a", 1, 1, 1,
                                             utils::PACKAGE_OPERATOR_EQUAL };
                pkg.conflicts = utils::PackagesLinkId(1, dep);
            }

            pkg.major = 1;
            pkg.minor = 2;
            pkg.patch = 3;

            ofs << pkg;
            OX << pkg;
        }
    }

    /* We need to ensure each file really installed. */
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    {
        utils::RemoteManager rmt(ctx);
        rmt.start(utils::REMOTE_MANAGER_SEARCH, ".*", nullptr);
        rmt.join();

        utils::Packages results;
        rmt.getResult(&results);

        BOOST_REQUIRE_EQUAL(results.empty(), false);
        BOOST_REQUIRE_EQUAL(results.size(), 10u);
    }

    {
        utils::RemoteManager rmt(ctx);
        rmt.start(utils::REMOTE_MANAGER_SEARCH, ".*", nullptr);
        rmt.join();

        utils::Packages results;
        rmt.getResult(&results);

        BOOST_REQUIRE_EQUAL(results.empty(), false);
        BOOST_REQUIRE_EQUAL(results.size(), 10u);
    }

    {
        utils::RemoteManager rmt(ctx);
        rmt.start(utils::REMOTE_MANAGER_SEARCH, ".*", nullptr);
        rmt.join();

        utils::Packages results;
        rmt.getResult(&results);

        BOOST_REQUIRE_EQUAL(results.empty(), false);
        BOOST_REQUIRE_EQUAL(results.size(), 10u);
    }

    vle::utils::RemoteManager rmt(ctx);

    std::ostringstream output;
    rmt.start(vle::utils::REMOTE_MANAGER_SHOW, "name-8", &output);
    rmt.join();

    {
        vle::utils::Packages pkgs;
        rmt.getResult(&pkgs);

        BOOST_REQUIRE_EQUAL(pkgs.size(), 1u);

        BOOST_REQUIRE_EQUAL(pkgs[0].name, "name-8");
        BOOST_REQUIRE_EQUAL(pkgs[0].size, 8u);
        BOOST_REQUIRE_EQUAL(pkgs[0].md5sum, "1234567890987654321");
        BOOST_REQUIRE_EQUAL(pkgs[0].url, "http://www.vle-project.org");
        BOOST_REQUIRE_EQUAL(pkgs[0].maintainer, "me");
        BOOST_REQUIRE_EQUAL(pkgs[0].major, 1);
        BOOST_REQUIRE_EQUAL(pkgs[0].minor, 2);
        BOOST_REQUIRE_EQUAL(pkgs[0].patch, 3);
        BOOST_REQUIRE_EQUAL(pkgs[0].description, "too good");
    }

    {
        vle::utils::Packages pkgs;
        rmt.start(vle::utils::REMOTE_MANAGER_SEARCH, ".*", &output);
        rmt.join();
        rmt.getResult(&pkgs);
        BOOST_REQUIRE_EQUAL(pkgs.size(), 10u);
    }

    {
        vle::utils::Packages pkgs;
        rmt.start(vle::utils::REMOTE_MANAGER_SEARCH, "name.*", &output);
        rmt.join();
        rmt.getResult(&pkgs);
        BOOST_REQUIRE_EQUAL(pkgs.size(), 10u);
    }

    {
        vle::utils::Packages pkgs;
        rmt.start(vle::utils::REMOTE_MANAGER_SEARCH, "0$", &output);
        rmt.join();
        rmt.getResult(&pkgs);
        BOOST_REQUIRE_EQUAL(pkgs.size(), 1u);
    }

    {
        vle::utils::Packages pkgs;
        rmt.start(vle::utils::REMOTE_MANAGER_SEARCH, ".*", &output);
        rmt.join();
        rmt.getResult(&pkgs);
        BOOST_REQUIRE_EQUAL(pkgs.size(), 10u);
    }

    {
        vle::utils::Packages pkgs;
        rmt.start(vle::utils::REMOTE_MANAGER_SEARCH, "[1|2]$", &output);
        rmt.join();
        rmt.getResult(&pkgs);
        BOOST_REQUIRE_EQUAL(pkgs.size(), 2u);
    }
}

BOOST_AUTO_TEST_CASE(test_compress_filepath)
{
    auto ctx = vle::utils::make_context();
    std::string filepath;
    std::string uniquepath;

    try {
        utils::Path unique = utils::Path::unique_path("copy-template");
        vle::utils::Package pkg(ctx, unique.string());
        pkg.create();
        pkg.wait(std::cerr, std::cerr);
        pkg.configure();
        pkg.wait(std::cerr, std::cerr);
        pkg.build();
        pkg.wait(std::cerr, std::cerr);
        pkg.install();
        filepath = pkg.getSrcDir(vle::utils::PKG_SOURCE);
        uniquepath = unique.string();
    } catch (...) {
        BOOST_REQUIRE(false);
    }

    /* We need to ensure each file really installed. */
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    BOOST_REQUIRE(not filepath.empty());

    utils::RemoteManager rmt(ctx);
    utils::Path tarfile(utils::Path::temp_directory_path());
    tarfile /= "check.tar.bz2";

    BOOST_REQUIRE_NO_THROW(rmt.compress(uniquepath, tarfile.string()));

    utils::Path t { tarfile };
    BOOST_REQUIRE(t.exists());

    utils::Path tmpfile(utils::Path::temp_directory_path());
    tmpfile /= "unique";

    tmpfile.create_directory();

    BOOST_REQUIRE_NO_THROW(rmt.decompress(tarfile.string(), tmpfile.string()));
    utils::Path t2 { tmpfile };
    BOOST_REQUIRE(t2.exists());

    t2 /= uniquepath;

    BOOST_REQUIRE(t2.exists());
}
