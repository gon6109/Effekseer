﻿
#ifndef __EFFEKSEER_INTERNAL_SCRIPT_H__
#define __EFFEKSEER_INTERNAL_SCRIPT_H__

#include <array>
#include <stdint.h>
#include <vector>

namespace Effekseer
{

class InternalScript
{
public:
	enum class RunningPhaseType : int32_t
	{
		Global,
		Local,
	};

private:
	enum class OperatorType : int32_t
	{
		Constant = 0,
		Add = 1,
		Sub = 2,
		Mul = 3,
		Div = 4,

		UnaryAdd = 11,
		UnarySub = 12,
	};

private:
	RunningPhaseType runningPhase = RunningPhaseType::Local;
	std::vector<float> registers;
	std::vector<uint8_t> operators;
	int32_t version_ = 0;
	int32_t operatorCount_ = 0;
	int32_t outputRegister_ = 0;
	bool isValid_ = false;

	bool IsValidOperator(int value) const;
	bool IsValidRegister(int index) const;
	float GetRegisterValue(int index,
						   const std::array<float, 4>& externals,
						   const std::array<float, 1>& globals,
						   const std::array<float, 5>& locals) const;

public:
	InternalScript();
	virtual ~InternalScript();
	bool Load(uint8_t* data, int size);
	float Execute(const std::array<float, 4>& externals, const std::array<float, 1>& globals, const std::array<float, 5>& locals);
	RunningPhaseType GetRunningPhase() const { return runningPhase; }
};

} // namespace Effekseer

#endif
