/**
 * @file src/vle/vpz/test/test1.cpp
 * @author The VLE Development Team
 */

/*
 * VLE Environment - the multimodeling and simulation environment
 * This file is a part of the VLE environment (http://vle.univ-littoral.fr)
 * Copyright (C) 2003 - 2008 The VLE Development Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_MAIN
#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE value_complete_test
#include <boost/test/unit_test.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <vle/vpz/Vpz.hpp>
#include <vle/vpz/SaxParser.hpp>
#include <vle/value/Boolean.hpp>
#include <vle/value/Integer.hpp>
#include <vle/value/Double.hpp>
#include <vle/value/String.hpp>
#include <vle/value/Set.hpp>
#include <vle/value/Map.hpp>
#include <vle/value/Matrix.hpp>
#include <vle/value/Tuple.hpp>
#include <vle/value/Table.hpp>
#include <vle/value/XML.hpp>
#include <vle/utils/Tools.hpp>
#include <limits>
#include <fstream>

using namespace vle;

BOOST_AUTO_TEST_CASE(build)
{
    //vpz::VLESaxParser sax;
    //sax.parse_memory("<?xml version=\"1.0\"?>\n<trame></trame>");
    //
    //BOOST_CHECK(sax.is_value() == false);
    //BOOST_CHECK(sax.get_values().empty() == true);
}

BOOST_AUTO_TEST_CASE(value_bool)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n<boolean>true</boolean>";
    const char* t2 = "<?xml version=\"1.0\"?>\n<boolean>false</boolean>";
    const char* t3 = "<?xml version=\"1.0\"?>\n<boolean>1</boolean>";
    const char* t4 = "<?xml version=\"1.0\"?>\n<boolean>0</boolean>";

    value::Value v;

    v = vpz::Vpz::parseValue(t1);
    BOOST_CHECK(value::toBoolean(v) == true);

    v = vpz::Vpz::parseValue(t2);
    BOOST_CHECK(value::toBoolean(v) == false);

    v = vpz::Vpz::parseValue(t3);
    BOOST_CHECK(value::toBoolean(v) == true);

    v = vpz::Vpz::parseValue(t4);
    BOOST_CHECK(value::toBoolean(v) == false);
}

BOOST_AUTO_TEST_CASE(value_integer)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n<integer>100</integer>";
    const char* t2 = "<?xml version=\"1.0\"?>\n<integer>-100</integer>";
    char t3[1000];
    char t4[1000];

    sprintf(t3, "<?xml version=\"1.0\"?>\n<integer>%s</integer>",
            utils::to_string(std::numeric_limits< long >::max()).c_str());
    sprintf(t4, "<?xml version=\"1.0\"?>\n<integer>%s</integer>",
            utils::to_string(std::numeric_limits< long >::min()).c_str());


    value::Value v;

    v = vpz::Vpz::parseValue(t1);
    BOOST_CHECK(value::toInteger(v) == 100);

    v = vpz::Vpz::parseValue(t2);
    BOOST_CHECK(value::toInteger(v) == -100);

    v = vpz::Vpz::parseValue(t3);
    BOOST_CHECK(value::toLong(v) ==
                std::numeric_limits < long >::max());

    v = vpz::Vpz::parseValue(t4);
    BOOST_CHECK(value::toLong(v) ==
                std::numeric_limits < long >::min());
}

BOOST_AUTO_TEST_CASE(value_double)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n<double>100.5</double>";
    const char* t2 = "<?xml version=\"1.0\"?>\n<double>-100.5</double>";

    value::Value v;

    v = vpz::Vpz::parseValue(t1);
    BOOST_CHECK_CLOSE(value::toDouble(v), 100.5, 1);

    v = vpz::Vpz::parseValue(t2);
    BOOST_CHECK_CLOSE(value::toDouble(v), -100.5, 1);
}

BOOST_AUTO_TEST_CASE(value_string)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n<string>a b c d e f g h i j</string>";
    const char* t2 = "<?xml version=\"1.0\"?>\n<string>a\nb\tc\n</string>";

    value::Value v;

    v = vpz::Vpz::parseValue(t1);
    BOOST_CHECK(value::toString(v) == "a b c d e f g h i j");

    v = vpz::Vpz::parseValue(t2);
    BOOST_CHECK(value::toString(v) == "a\nb\tc\n");
}

BOOST_AUTO_TEST_CASE(value_set)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n"
        "<set>\n"
        "<integer>1</integer>\n"
        "<string>test</string>\n"
        "</set>";

    const char* t2 = "<?xml version=\"1.0\"?>\n"
        "<set>\n"
        "  <integer>1</integer>\n"
        "  <set>\n"
        "    <string>test</string>\n"
        "  </set>\n"
        "</set>";

    value::Set v;

    v = value::toSetValue(vpz::Vpz::parseValue(t1));
    BOOST_CHECK(value::toString(v->getValue(1)) == "test");
    BOOST_CHECK(value::toInteger(v->getValue(0)) == 1);

    v = value::toSetValue(vpz::Vpz::parseValue(t2));
    BOOST_CHECK(value::toInteger(v->getValue(0)) == 1);
    value::Set v2 = value::toSetValue(v->getValue(1));
    BOOST_CHECK(value::toString(v2->getValue(0)) == "test");
}

BOOST_AUTO_TEST_CASE(value_map)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n"
        "<map>\n"
        " <key name=\"a\">\n"
        "  <integer>10</integer>\n"
        " </key>\n"
        "</map>\n";

    const char* t2 = "<?xml version=\"1.0\"?>\n"
        "<map>\n"
        " <key name=\"a\">\n"
        "  <set>\n"
        "   <integer>1</integer>\n"
        "   <string>test</string>\n"
        "  </set>\n"
        " </key>\n"
        "</map>\n";

    value::Map v;

    v = value::toMapValue(vpz::Vpz::parseValue(t1));
    BOOST_CHECK(value::toInteger(v->getValue("a")) == 10);

    v = value::toMapValue(vpz::Vpz::parseValue(t2));
    value::Set s;

    s = value::toSetValue(v->getValue("a"));
    BOOST_CHECK(value::toInteger(s->getValue(0)) == 1);
    BOOST_CHECK(value::toString(s->getValue(1)) == "test");
}

BOOST_AUTO_TEST_CASE(value_tuple)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n"
        "<tuple>1 2 3</tuple>\n";

    value::Tuple v;

    v = value::toTupleValue(vpz::Vpz::parseValue(t1));
    BOOST_REQUIRE_EQUAL(v->size(), (size_t)3);
    BOOST_CHECK_CLOSE(v->operator[](0), 1.0, 0.1);
    BOOST_CHECK_CLOSE(v->operator[](1), 2.0, 0.1);
    BOOST_CHECK_CLOSE(v->operator[](2), 3.0, 0.1);

    const char* t2 = "<?xml version=\"1.0\"?>\n"
        "<map>\n"
        "   <key name=\"testtest\">\n"
        "      <tuple>100 200 300</tuple>\n"
        "   </key>\n"
        "</map>\n";

    value::Map m;
    m = value::toMapValue(vpz::Vpz::parseValue(t2));

    value::Value t = m->getValue("testtest");
    v = toTupleValue(t);
    BOOST_CHECK_CLOSE(v->operator[](0), 100.0, 0.1);
    BOOST_CHECK_CLOSE(v->operator[](1), 200.0, 0.1);
    BOOST_CHECK_CLOSE(v->operator[](2), 300.0, 0.1);
}

BOOST_AUTO_TEST_CASE(value_table)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n"
        "<table width=\"2\" height=\"3\">\n"
        "1 2 3 4 5 6"
        "</table>\n";

    value::Table v;

    v = value::toTableValue(vpz::Vpz::parseValue(t1));
    BOOST_REQUIRE_EQUAL(v->width(), (value::TableFactory::index)2);
    BOOST_REQUIRE_EQUAL(v->height(), (value::TableFactory::index)3);
    BOOST_CHECK_CLOSE(v->get(0, 0), 1.0, 0.1);
    BOOST_CHECK_CLOSE(v->get(0, 1), 3.0, 0.1);
    BOOST_CHECK_CLOSE(v->get(0, 2), 5.0, 0.1);
    BOOST_CHECK_CLOSE(v->get(1, 0), 2.0, 0.1);
    BOOST_CHECK_CLOSE(v->get(1, 1), 4.0, 0.1);
    BOOST_CHECK_CLOSE(v->get(1, 2), 6.0, 0.1);

    BOOST_REQUIRE_EQUAL(v->toString(), "((1,2),(3,4),(5,6))");

    std::string result("<?xml version=\"1.0\"?>\n");
    result += v->toXML();
    value::Table w;
    w = value::toTableValue(vpz::Vpz::parseValue(result));
    BOOST_REQUIRE_EQUAL(w->width(), (value::TableFactory::index)2);
    BOOST_REQUIRE_EQUAL(w->height(), (value::TableFactory::index)3);
    BOOST_CHECK_CLOSE(w->get(0, 0), 1.0, 0.1);
    BOOST_CHECK_CLOSE(w->get(0, 1), 3.0, 0.1);
    BOOST_CHECK_CLOSE(w->get(0, 2), 5.0, 0.1);
    BOOST_CHECK_CLOSE(w->get(1, 0), 2.0, 0.1);
    BOOST_CHECK_CLOSE(w->get(1, 1), 4.0, 0.1);
    BOOST_CHECK_CLOSE(w->get(1, 2), 6.0, 0.1);
}

BOOST_AUTO_TEST_CASE(value_xml)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n"
        "<xml>"
        "<![CDATA[test 1 2 1 2]]>\n"
        "</xml>";

    value::XML v;

    BOOST_REQUIRE_EQUAL(value::toXml(vpz::Vpz::parseValue(t1)), "test 1 2 1 2");
}

BOOST_AUTO_TEST_CASE(value_matrix)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n"
        "<matrix rows=\"3\" columns=\"2\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<integer>1</integer>"
        "<integer>2</integer>"
        "<integer>3</integer>"
        "<integer>4</integer>"
        "<integer>5</integer>"
        "<integer>6</integer>"
        "</matrix>";

    value::Value v = vpz::Vpz::parseValue(t1);
    BOOST_REQUIRE_EQUAL(v->isMatrix(), true);

    value::Matrix m = value::toMatrixValue(v);
    value::MatrixFactory::MatrixView mv = value::toMatrix(v);

    BOOST_REQUIRE_EQUAL(m->rows(), (value::MatrixFactory::size_type)3);
    BOOST_REQUIRE_EQUAL(m->columns(), (value::MatrixFactory::size_type)2);

    BOOST_REQUIRE_EQUAL(mv[0][0]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv[0][1]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv[0][2]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv[1][0]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv[1][1]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv[1][2]->isInteger(), true);

    BOOST_REQUIRE_EQUAL(value::toInteger(mv[0][0]), 1);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv[1][0]), 2);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv[0][1]), 3);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv[1][1]), 4);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv[0][2]), 5);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv[1][2]), 6);

    std::string result("<?xml version=\"1.0\"?>\n");
    result += v->toXML();
    value::Value v2 = vpz::Vpz::parseValue(result);
    BOOST_REQUIRE_EQUAL(v2->isMatrix(), true);
    value::Matrix m2 = value::toMatrixValue(v2);
    value::MatrixFactory::MatrixView mv2 = value::toMatrix(v2);

    BOOST_REQUIRE_EQUAL(m2->rows(), (value::MatrixFactory::size_type)3);
    BOOST_REQUIRE_EQUAL(m2->columns(), (value::MatrixFactory::size_type)2);

    BOOST_REQUIRE_EQUAL(mv2[0][0]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv2[0][1]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv2[0][2]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv2[1][0]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv2[1][1]->isInteger(), true);
    BOOST_REQUIRE_EQUAL(mv2[1][2]->isInteger(), true);

    BOOST_REQUIRE_EQUAL(value::toInteger(mv2[0][0]), 1);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv2[1][0]), 2);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv2[0][1]), 3);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv2[1][1]), 4);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv2[0][2]), 5);
    BOOST_REQUIRE_EQUAL(value::toInteger(mv2[1][2]), 6);
}


BOOST_AUTO_TEST_CASE(value_matrix_of_matrix)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n"
        "<matrix rows=\"1\" columns=\"3\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<matrix rows=\"1\" columns=\"3\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<integer>1</integer>"
        "<integer>2</integer>"
        "<integer>3</integer>"
        "</matrix>"
        "<matrix rows=\"1\" columns=\"3\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<integer>4</integer>"
        "<integer>5</integer>"
        "<integer>6</integer>"
        "</matrix>"
        "<matrix rows=\"1\" columns=\"3\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<integer>7</integer>"
        "<integer>8</integer>"
        "<integer>9</integer>"
        "</matrix>"
        "</matrix>";

    value::Value v = vpz::Vpz::parseValue(t1);
    BOOST_REQUIRE_EQUAL(v->isMatrix(), true);

    value::Matrix m = value::toMatrixValue(v);
    value::MatrixFactory::MatrixView mv = value::toMatrix(v);

    BOOST_REQUIRE_EQUAL(m->rows(), (value::MatrixFactory::size_type)1);
    BOOST_REQUIRE_EQUAL(m->columns(), (value::MatrixFactory::size_type)3);

    BOOST_REQUIRE_EQUAL(mv[0][0]->isMatrix(), true);
    BOOST_REQUIRE_EQUAL(mv[1][0]->isMatrix(), true);
    BOOST_REQUIRE_EQUAL(mv[2][0]->isMatrix(), true);

    value::MatrixFactory::MatrixView m1 = value::toMatrix(mv[0][0]);
    value::MatrixFactory::MatrixView m2 = value::toMatrix(mv[1][0]);
    value::MatrixFactory::MatrixView m3 = value::toMatrix(mv[2][0]);
    BOOST_REQUIRE_EQUAL(value::toInteger(m1[0][0]), 1);
    BOOST_REQUIRE_EQUAL(value::toInteger(m1[1][0]), 2);
    BOOST_REQUIRE_EQUAL(value::toInteger(m1[2][0]), 3);
    BOOST_REQUIRE_EQUAL(value::toInteger(m2[0][0]), 4);
    BOOST_REQUIRE_EQUAL(value::toInteger(m2[1][0]), 5);
    BOOST_REQUIRE_EQUAL(value::toInteger(m2[2][0]), 6);
    BOOST_REQUIRE_EQUAL(value::toInteger(m3[0][0]), 7);
    BOOST_REQUIRE_EQUAL(value::toInteger(m3[1][0]), 8);
    BOOST_REQUIRE_EQUAL(value::toInteger(m3[2][0]), 9);
}

BOOST_AUTO_TEST_CASE(value_matrix_of_matrix_io)
{
    const char* t1 = "<?xml version=\"1.0\"?>\n"
        "<matrix rows=\"1\" columns=\"3\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<matrix rows=\"1\" columns=\"3\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<null />"
        "<integer>2</integer>"
        "<integer>3</integer>"
        "</matrix>"
        "<matrix rows=\"1\" columns=\"3\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<integer>4</integer>"
        "<integer>5</integer>"
        "<null />"
        "</matrix>"
        "<matrix rows=\"1\" columns=\"3\" columnmax=\"15\" "
        "        rowmax=\"25\" columnstep=\"100\" rowstep=\"200\" >"
        "<integer>7</integer>"
        "<null />"
        "<integer>9</integer>"
        "</matrix>"
        "</matrix>";

    value::Value v = vpz::Vpz::parseValue(t1);

    std::string str(v->toXML());
    str = "<?xml version=\"1.0\"?>\n" + str;

    value::Value v2 = vpz::Vpz::parseValue(str);
    BOOST_REQUIRE_EQUAL(v2->isMatrix(), true);

    value::Matrix m = value::toMatrixValue(v2);
    value::MatrixFactory::MatrixView mv = value::toMatrix(v2);

    BOOST_REQUIRE_EQUAL(m->rows(), (value::MatrixFactory::size_type)1);
    BOOST_REQUIRE_EQUAL(m->columns(), (value::MatrixFactory::size_type)3);

    BOOST_REQUIRE_EQUAL(mv[0][0]->isMatrix(), true);
    BOOST_REQUIRE_EQUAL(mv[1][0]->isMatrix(), true);
    BOOST_REQUIRE_EQUAL(mv[2][0]->isMatrix(), true);

    value::MatrixFactory::MatrixView m1 = value::toMatrix(mv[0][0]);
    value::MatrixFactory::MatrixView m2 = value::toMatrix(mv[1][0]);
    value::MatrixFactory::MatrixView m3 = value::toMatrix(mv[2][0]);
    BOOST_REQUIRE_EQUAL(m1[0][0].get(), (value::ValueBase*)0);
    BOOST_REQUIRE_EQUAL(value::toInteger(m1[1][0]), 2);
    BOOST_REQUIRE_EQUAL(value::toInteger(m1[2][0]), 3);
    BOOST_REQUIRE_EQUAL(value::toInteger(m2[0][0]), 4);
    BOOST_REQUIRE_EQUAL(value::toInteger(m2[1][0]), 5);
    BOOST_REQUIRE_EQUAL(m2[2][0].get(), (value::ValueBase*)0);
    BOOST_REQUIRE_EQUAL(value::toInteger(m3[0][0]), 7);
    BOOST_REQUIRE_EQUAL(m3[1][0].get(), (value::ValueBase*)0);
    BOOST_REQUIRE_EQUAL(value::toInteger(m3[2][0]), 9);
}
