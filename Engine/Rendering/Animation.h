#pragma once

#include "../Resource/IResource.h"
#include "../Math/Matrix.h"

namespace PlayGround
{
	struct AnimationVertexWeight
	{
		uint32_t vertexID;
		float weight;
	};

	struct AnimationBone
	{
		std::string name;
		std::vector<AnimationVertexWeight> vertexWeights;
		Math::Matrix offset;
	};

	struct KeyVector
	{
		double time;
		Math::Vector3 value;
	};

	struct KeyQuaternion
	{
		double time;
		Math::Quaternion value;
	};

	struct AnimationNode
	{
		std::string name;
		std::vector<KeyVector> positionFrames;
		std::vector<KeyQuaternion> rotationFrames;
		std::vector<KeyVector> scaleFrames;
	};

	class Animation : public IResource
	{
	public:
		Animation(Context* context);
		~Animation() = default;

		bool LoadFromFile(const std::string& file_path) override;
		bool SaveToFile(const std::string& file_path) override;

		inline void SetName(const std::string& name)
		{
			m_Object_name = name;
		}

		inline void SetDuration(double duration) 
		{
			m_Duration = duration;
		}

		inline void SetTicksPerSec(double ticksPerSec)
		{
			m_TicksPerSec = ticksPerSec;
		}

	private:
		std::string m_Object_name;
		double m_Duration = 0;
		double m_TicksPerSec = 0;

		std::vector<AnimationNode> m_vecChannels;
	};
}
