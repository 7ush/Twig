/* Copyright (c) 2014, The Nuria Project
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *    1. The origin of this software must not be misrepresented; you must not
 *       claim that you wrote the original software. If you use this software
 *       in a product, an acknowledgment in the product documentation would be
 *       appreciated but is not required.
 *    2. Altered source versions must be plainly marked as such, and must not be
 *       misrepresented as being the original software.
 *    3. This notice may not be removed or altered from any source
 *       distribution.
 */

#include "private/tokenizer.hpp"
#include <nuria/debug.hpp>
#include <QtTest/QTest>

#include "grammar/twig.h"

using namespace Nuria::Template;
using namespace Nuria;

// For CHECK_TOKEN(_VALUE)
static bool operator!= (const Template::Location &lhs, const Template::Location &rhs)
{ return !(lhs.row == rhs.row && lhs.column == rhs.column); }

// For easier debugging of location issues
static inline QDebug operator<< (QDebug dbg, const Template::Location &loc)
{ return dbg << "[" << loc.row << "|" << loc.column << "]"; }

// Helper macros
#define CHECK_TOKEN(Tokenizer, Type, Row, Column) \
{ \
	Token tok = Tokenizer.nextToken (); \
	if (tok.tokenId != Type || tok.row != Row || tok.column != Column) { \
	qWarning() << "Result  :" << tok; \
	qWarning() << "Expected:" << Token (Type, Row, Column); \
	QFAIL("The returned token did not match the expected one."); \
	} \
	}

#define CHECK_TOKEN_VALUE(Tokenizer, Type, Row, Column, Value) \
{ \
	Token tok = Tokenizer.nextToken (); \
	if (tok.tokenId != Type || tok.row != Row || tok.column != Column || tok.value != Value) { \
	qWarning() << "Result  :" << tok; \
	qWarning() << "Expected:" << Token (Type, Row, Column, Value); \
	QFAIL("The returned token did not match the expected one."); \
	} \
	}

// 
class TemplateTokenizerTest : public QObject {
	Q_OBJECT
private slots:
	
	void emptyInput ();
	void textOnly ();
	void commandOnly ();
	void expansionOnly ();
	void commentOnly ();
	void commandAfterCommand ();
	void textAndExpansions ();
	
	// 
	void testBasicTypes_data ();
	void testBasicTypes ();
	
	// 
	void singleTokens_data ();
	void singleTokens ();
	void commandTokens_data ();
	void commandTokens ();
	
	// 
	void testIn ();
	void expansionWithFilters ();
};

void TemplateTokenizerTest::emptyInput () {
	Template::Tokenizer tokenizer;
	
	tokenizer.read ("");
	
	QVERIFY(tokenizer.allTokens ().isEmpty ());
	QCOMPARE(tokenizer.pos (), 0);
}

void TemplateTokenizerTest::textOnly () {
	Template::Tokenizer tokenizer;
	tokenizer.read ("Yadda");
	
	QCOMPARE(tokenizer.allTokens ().length (), 1);
	CHECK_TOKEN_VALUE(tokenizer, TOK_TEXT, 0, 0, "Yadda");
	QCOMPARE(tokenizer.pos (), 1);
	QVERIFY(tokenizer.atEnd ());
}

void TemplateTokenizerTest::commandOnly () {
	Template::Tokenizer tokenizer;
	tokenizer.read ("{% if %}");
	
	QCOMPARE(tokenizer.allTokens ().length (), 3);
	
	CHECK_TOKEN(tokenizer, TOK_COMMAND_BEGIN, 0, 0);
	CHECK_TOKEN_VALUE(tokenizer, TOK_IF_BEGIN, 0, 3, "if");
	CHECK_TOKEN(tokenizer, TOK_COMMAND_END, 0, 6);
	
	QCOMPARE(tokenizer.pos (), 3);
	QVERIFY(tokenizer.atEnd ());
	
}


void TemplateTokenizerTest::expansionOnly () {
	Template::Tokenizer tokenizer;
	tokenizer.read ("{{ yadda }}");
	
	QCOMPARE(tokenizer.allTokens ().length (), 3);
	
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_BEGIN, 0, 0);
	CHECK_TOKEN_VALUE(tokenizer, TOK_SYMBOL, 0, 3, "yadda");
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_END, 0, 9);
	
	QCOMPARE(tokenizer.pos (), 3);
	QVERIFY(tokenizer.atEnd ());
	
}

void TemplateTokenizerTest::commentOnly () {
	Template::Tokenizer tokenizer;
	tokenizer.read ("{# yadda #}");
	
	QCOMPARE(tokenizer.allTokens ().length (), 0);
	QVERIFY(tokenizer.atEnd ());
	
}

void TemplateTokenizerTest::commandAfterCommand () {
	Template::Tokenizer tokenizer;
	tokenizer.read ("{% if %}{% endif %}");
	
	QCOMPARE(tokenizer.allTokens ().length (), 6);
	
	CHECK_TOKEN(tokenizer, TOK_COMMAND_BEGIN, 0, 0);
	CHECK_TOKEN_VALUE(tokenizer, TOK_IF_BEGIN, 0, 3, "if");
	CHECK_TOKEN(tokenizer, TOK_COMMAND_END, 0, 6);
	
	CHECK_TOKEN(tokenizer, TOK_COMMAND_BEGIN, 0, 8);
	CHECK_TOKEN_VALUE(tokenizer, TOK_IF_END, 0, 11, "endif");
	CHECK_TOKEN(tokenizer, TOK_COMMAND_END, 0, 17);
	
	QCOMPARE(tokenizer.pos (), 6);
	QVERIFY(tokenizer.atEnd ());
}

void TemplateTokenizerTest::textAndExpansions () {
	Template::Tokenizer tokenizer;
	tokenizer.read ("front{{ foo }}middle{{ bar }}end");
	
	QCOMPARE(tokenizer.allTokens ().length (), 9);
	
	CHECK_TOKEN_VALUE(tokenizer, TOK_TEXT, 0, 0, "front");
	
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_BEGIN, 0, 5);
	CHECK_TOKEN_VALUE(tokenizer, TOK_SYMBOL, 0, 8, "foo");
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_END, 0, 12);
	
	CHECK_TOKEN_VALUE(tokenizer, TOK_TEXT, 0,14, "middle");
	
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_BEGIN, 0, 20);
	CHECK_TOKEN_VALUE(tokenizer, TOK_SYMBOL, 0, 23, "bar");
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_END, 0, 27);
	
	CHECK_TOKEN_VALUE(tokenizer, TOK_TEXT, 0, 29, "end");
	
	QCOMPARE(tokenizer.pos (), 9);
	QVERIFY(tokenizer.atEnd ());
}

void TemplateTokenizerTest::testBasicTypes_data () {
	QTest::addColumn< QString > ("input");
	QTest::addColumn< int > ("token");
	QTest::addColumn< QVariant > ("result");
	
	QTest::newRow("integer") << "{{ 123 }}" << TOK_INTEGER << QVariant (123);
	QTest::newRow("number") << "{{ 12.34 }}" << TOK_NUMBER << QVariant (double (12.34));
	QTest::newRow("number w/ dot") << "{{ 12. }}" << TOK_NUMBER << QVariant (double (12));
	QTest::newRow("number w/ exponent w/ decimal") << "{{ 12.34e2 }}" << TOK_NUMBER << QVariant (double (1234));
	QTest::newRow("number w/ exponent w/ dot") << "{{ 12.e2 }}" << TOK_NUMBER << QVariant (double (1200));
	QTest::newRow("number w/ exponent") << "{{ 12e2 }}" << TOK_NUMBER << QVariant (double (1200));
	QTest::newRow("empty string") << "{{ \"\" }}" << TOK_STRING << QVariant ("\"");
	QTest::newRow("single-quote empty") << "{{ '' }}" << TOK_STRING << QVariant ("'");
	QTest::newRow("string") << "{{ \"foo\\\"bar'\\\\\" }}" << TOK_STRING << QVariant ("\"foo\"bar'\\");
	QTest::newRow("single-quote") << "{{ 'foo\"bar\\'\\\\' }}" << TOK_STRING << QVariant ("'foo\"bar'\\");
	QTest::newRow("true") << "{{ true }}" << TOK_TRUE << QVariant (true);
	QTest::newRow("false") << "{{ false }}" << TOK_FALSE << QVariant (false);
	
}

void TemplateTokenizerTest::testBasicTypes () {
	QFETCH(QString, input);
	QFETCH(int, token);
	QFETCH(QVariant, result);
	
	Template::Tokenizer tokenizer;
	tokenizer.read (input.toLatin1 ());
	
//	QCOMPARE(tokenizer.allTokens ().length (), 3);
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_BEGIN, 0, 0);
	CHECK_TOKEN_VALUE(tokenizer, token, 0, 3, result);
	
}

void TemplateTokenizerTest::singleTokens_data () {
	QTest::addColumn< QString > ("input");
	QTest::addColumn< int > ("token");
	
	QTest::newRow("assign") << "=" << TOK_ASSIGN;
	QTest::newRow("not") << "not" << TOK_NOT;
	QTest::newRow("in") << "in" << TOK_IN;
	QTest::newRow("==") << "==" << TOK_EQUALS;
	QTest::newRow("!=") << "!=" << TOK_NOT_EQUALS;
	QTest::newRow("<") << "<" << TOK_LESS;
	QTest::newRow("<=") << "<=" << TOK_LESS_EQUAL;
	QTest::newRow(">") << ">" << TOK_GREATER;
	QTest::newRow(">=") << ">=" << TOK_GREATER_EQUAL;
	QTest::newRow("[") << "[" << TOK_ARRAY_BEGIN;
	QTest::newRow("]") << "]" << TOK_ARRAY_END;
	QTest::newRow("(") << "(" << TOK_PAREN_OPEN;
	QTest::newRow(")") << ")" << TOK_PAREN_CLOSE;
	QTest::newRow("..") << ".." << TOK_PERIOD_PERIOD;
	QTest::newRow(".") << "." << TOK_PERIOD;
	QTest::newRow("comma") << "," << TOK_COMMA;
	QTest::newRow("colon") << ":" << TOK_COLON;
	QTest::newRow("pipe") << "|" << TOK_PIPE;
	
//	QTest::newRow("") << "" << TOK_;
}

void TemplateTokenizerTest::singleTokens () {
	QFETCH(QString, input);
	QFETCH(int, token);
	
	int tokLen = input.length ();
	input.prepend ("{{");
	input.append ("}}");
	
	Template::Tokenizer tokenizer;
	tokenizer.read (input.toLatin1 ());
	
	QCOMPARE(tokenizer.allTokens ().length (), 3);
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_BEGIN, 0, 0);
	CHECK_TOKEN(tokenizer, token, 0, 2);
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_END, 0, 2 + tokLen);
	
}

void TemplateTokenizerTest::commandTokens_data () {
	QTest::addColumn< QString > ("input");
	QTest::addColumn< int > ("token");
	
	QTest::newRow("set") << "set" << TOK_SET;
	QTest::newRow("extends") << "extends" << TOK_EXTENDS;
	QTest::newRow("include") << "include" << TOK_INCLUDE;
	QTest::newRow("block") << "block" << TOK_BLOCK_BEGIN;
	QTest::newRow("endblock") << "endblock" << TOK_BLOCK_END;
	QTest::newRow("for") << "for" << TOK_FOR_BEGIN;
	QTest::newRow("endfor") << "endfor" << TOK_FOR_END;
	QTest::newRow("if") << "if" << TOK_IF_BEGIN;
	QTest::newRow("endif") << "endif" << TOK_IF_END;
	QTest::newRow("else") << "else" << TOK_ELSE;
	QTest::newRow("autoescape") << "autoescape" << TOK_AUTOESCAPE_BEGIN;
	QTest::newRow("endautoescape") << "endautoescape" << TOK_AUTOESCAPE_END;
	QTest::newRow("spaceless") << "spaceless" << TOK_SPACELESS_BEGIN;
	QTest::newRow("endspaceless") << "endspaceless" << TOK_SPACELESS_END;
//	QTest::newRow("") << "" << TOK_;
	
}

void TemplateTokenizerTest::commandTokens () {
	QFETCH(QString, input);
	QFETCH(int, token);
	
	int tokLen = input.length ();
	input.prepend ("{%");
	input.append ("%}");
	
	Template::Tokenizer tokenizer;
	tokenizer.read (input.toLatin1 ());
	
	QCOMPARE(tokenizer.allTokens ().length (), 3);
	CHECK_TOKEN(tokenizer, TOK_COMMAND_BEGIN, 0, 0);
	CHECK_TOKEN(tokenizer, token, 0, 2);
	CHECK_TOKEN(tokenizer, TOK_COMMAND_END, 0, 2 + tokLen);
}

void TemplateTokenizerTest::testIn () {
	Template::Tokenizer tokenizer;
	tokenizer.read ("{{ \"foo\" in [ 1, 2 ] }}");
	
	QCOMPARE(tokenizer.allTokens ().length (), 9);
	
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_BEGIN, 0, 0);
	
	CHECK_TOKEN_VALUE(tokenizer, TOK_STRING, 0, 3, "\"foo");
	CHECK_TOKEN(tokenizer, TOK_IN, 0, 9);
	CHECK_TOKEN(tokenizer, TOK_ARRAY_BEGIN, 0,12);
	CHECK_TOKEN_VALUE(tokenizer, TOK_INTEGER, 0, 14, 1);
	CHECK_TOKEN(tokenizer, TOK_COMMA, 0, 15);
	CHECK_TOKEN_VALUE(tokenizer, TOK_INTEGER, 0, 17, 2);
	CHECK_TOKEN(tokenizer, TOK_ARRAY_END, 0, 19);
	
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_END, 0, 21);
}

void TemplateTokenizerTest::expansionWithFilters () {
	Template::Tokenizer tokenizer;
	tokenizer.read ("{{ \"foo\"|foo(1,2)|bar }}");
	
	QCOMPARE(tokenizer.allTokens ().length (), 12);
	
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_BEGIN, 0, 0);
	CHECK_TOKEN_VALUE(tokenizer, TOK_STRING, 0, 3, "\"foo");
	CHECK_TOKEN(tokenizer, TOK_PIPE, 0, 8);
	CHECK_TOKEN_VALUE(tokenizer, TOK_SYMBOL, 0, 9, "foo");
	CHECK_TOKEN(tokenizer, TOK_PAREN_OPEN, 0, 12);
	CHECK_TOKEN_VALUE(tokenizer, TOK_INTEGER, 0, 13, 1);
	CHECK_TOKEN(tokenizer, TOK_COMMA, 0, 14);
	CHECK_TOKEN_VALUE(tokenizer, TOK_INTEGER, 0, 15, 2);
	CHECK_TOKEN(tokenizer, TOK_PAREN_CLOSE, 0, 16);
	CHECK_TOKEN(tokenizer, TOK_PIPE, 0, 17);
	CHECK_TOKEN_VALUE(tokenizer, TOK_SYMBOL, 0, 18, "bar");
	CHECK_TOKEN(tokenizer, TOK_EXPANSION_END, 0, 22);
	
}

QTEST_MAIN(TemplateTokenizerTest)
#include "tst_templatetokenizer.moc"