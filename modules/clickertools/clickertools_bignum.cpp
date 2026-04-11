/**************************************************************************/
/*  clickertools_bignum.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             BLAZIUM ENGINE                             */
/*                          https://blazium.app                           */
/**************************************************************************/
/* Copyright (c) 2024-present Blazium Engine contributors.                */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "clickertools_bignum.h"

BlaziumBigNum::BlaziumBigNum() {
	bignum = BigNumber::BigNum();
}

BlaziumBigNum::BlaziumBigNum(BigNumber::BigNum p_bignum) {
	bignum = p_bignum;
}

void BlaziumBigNum::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add", "other"), &BlaziumBigNum::add);
	ClassDB::bind_method(D_METHOD("sub", "other"), &BlaziumBigNum::sub);
	ClassDB::bind_method(D_METHOD("mul", "other"), &BlaziumBigNum::mul);
	ClassDB::bind_method(D_METHOD("div", "other"), &BlaziumBigNum::div);
	ClassDB::bind_method(D_METHOD("abs_num"), &BlaziumBigNum::abs_num);
	ClassDB::bind_method(D_METHOD("negate"), &BlaziumBigNum::negate);

	ClassDB::bind_method(D_METHOD("is_greater_than", "other"), &BlaziumBigNum::is_greater_than);
	ClassDB::bind_method(D_METHOD("is_less_than", "other"), &BlaziumBigNum::is_less_than);
	ClassDB::bind_method(D_METHOD("is_equal_to", "other"), &BlaziumBigNum::is_equal_to);

	ClassDB::bind_method(D_METHOD("is_inf"), &BlaziumBigNum::is_inf);
	ClassDB::bind_method(D_METHOD("is_nan"), &BlaziumBigNum::is_nan);
	ClassDB::bind_method(D_METHOD("is_positive"), &BlaziumBigNum::is_positive);
	ClassDB::bind_method(D_METHOD("is_negative"), &BlaziumBigNum::is_negative);

	ClassDB::bind_method(D_METHOD("get_mantissa"), &BlaziumBigNum::get_mantissa);
	ClassDB::bind_method(D_METHOD("get_exponent"), &BlaziumBigNum::get_exponent);

	ClassDB::bind_method(D_METHOD("as_string", "precision"), &BlaziumBigNum::as_string, DEFVAL(3));
	ClassDB::bind_method(D_METHOD("to_pretty_string", "precision"), &BlaziumBigNum::to_pretty_string, DEFVAL(3));

	ClassDB::bind_method(D_METHOD("parse_string", "value"), &BlaziumBigNum::parse_string);
	ClassDB::bind_method(D_METHOD("parse_float", "value"), &BlaziumBigNum::parse_float);

	ClassDB::bind_static_method("BlaziumBigNum", D_METHOD("from_string", "value"), &BlaziumBigNum::from_string);
	ClassDB::bind_static_method("BlaziumBigNum", D_METHOD("from_float", "value"), &BlaziumBigNum::from_float);
	ClassDB::bind_static_method("BlaziumBigNum", D_METHOD("from_bignum", "value"), &BlaziumBigNum::from_bignum);
}

Ref<BlaziumBigNum> BlaziumBigNum::add(Ref<BlaziumBigNum> p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), memnew(BlaziumBigNum(bignum)));
	return memnew(BlaziumBigNum(bignum.add(p_other->get_bignum())));
}

Ref<BlaziumBigNum> BlaziumBigNum::sub(Ref<BlaziumBigNum> p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), memnew(BlaziumBigNum(bignum)));
	return memnew(BlaziumBigNum(bignum.sub(p_other->get_bignum())));
}

Ref<BlaziumBigNum> BlaziumBigNum::mul(Ref<BlaziumBigNum> p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), memnew(BlaziumBigNum(bignum)));
	return memnew(BlaziumBigNum(bignum.mul(p_other->get_bignum())));
}

Ref<BlaziumBigNum> BlaziumBigNum::div(Ref<BlaziumBigNum> p_other) const {
	ERR_FAIL_COND_V(p_other.is_null(), memnew(BlaziumBigNum(bignum)));
	return memnew(BlaziumBigNum(bignum.div(p_other->get_bignum())));
}

Ref<BlaziumBigNum> BlaziumBigNum::abs_num() const {
	return memnew(BlaziumBigNum(bignum.abs()));
}

Ref<BlaziumBigNum> BlaziumBigNum::negate() const {
	return memnew(BlaziumBigNum(bignum.negate()));
}

bool BlaziumBigNum::is_greater_than(Ref<BlaziumBigNum> p_other) const {
	if (p_other.is_null()) {
		return false;
	}
	return bignum > p_other->get_bignum();
}

bool BlaziumBigNum::is_less_than(Ref<BlaziumBigNum> p_other) const {
	if (p_other.is_null()) {
		return false;
	}
	return bignum < p_other->get_bignum();
}

bool BlaziumBigNum::is_equal_to(Ref<BlaziumBigNum> p_other) const {
	if (p_other.is_null()) {
		return false;
	}
	return bignum == p_other->get_bignum();
}

bool BlaziumBigNum::is_inf() const {
	return bignum.is_inf();
}

bool BlaziumBigNum::is_nan() const {
	return bignum.is_nan();
}

bool BlaziumBigNum::is_positive() const {
	return bignum.is_positive();
}

bool BlaziumBigNum::is_negative() const {
	return bignum.is_negative();
}

double BlaziumBigNum::get_mantissa() const {
	return bignum.getM();
}

int64_t BlaziumBigNum::get_exponent() const {
	return bignum.getE();
}

String BlaziumBigNum::as_string(int p_precision) const {
	return String::utf8(bignum.to_string(p_precision).c_str());
}

String BlaziumBigNum::to_pretty_string(int p_precision) const {
	return String::utf8(bignum.to_pretty_string(p_precision).c_str());
}

void BlaziumBigNum::parse_string(const String &p_string) {
	CharString utf8 = p_string.utf8();
	std::string_view sv(utf8.get_data(), utf8.length());
	bignum = BigNumber::BigNum(sv);
}

void BlaziumBigNum::parse_float(double p_val) {
	bignum = BigNumber::BigNum(p_val);
}

Ref<BlaziumBigNum> BlaziumBigNum::from_string(const String &p_string) {
	Ref<BlaziumBigNum> b;
	b.instantiate();
	b->parse_string(p_string);
	return b;
}

Ref<BlaziumBigNum> BlaziumBigNum::from_float(double p_val) {
	Ref<BlaziumBigNum> b;
	b.instantiate();
	b->parse_float(p_val);
	return b;
}

Ref<BlaziumBigNum> BlaziumBigNum::from_bignum(const Ref<BlaziumBigNum> &p_other) {
	Ref<BlaziumBigNum> b;
	b.instantiate();
	if (p_other.is_valid()) {
		b->bignum = p_other->get_bignum();
	}
	return b;
}
