#pragma once

#include "EngineDefinition.h"

#include <string>
#include <algorithm>
#include <type_traits>
#include <memory>
#include <fstream>
#include <sstream>
#include <limits>
#include <cassert>
#include <cstdint>
#include <array>
#include <atomic>
#include <map>
#include <unordered_map>


#include "Core/Engine.h"
#include "Core/EventSystem.h"
#include "Core/Settings.h"
#include "Core/Context.h"
#include "Core/Timer.h"
#include "Core/FileSystem.h"
#include "Core/StopWatch.h"


#include "Log/Logger.h"

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Ray.h"
#include "Math/RayHit.h"
#include "Math/Rectangle.h"
#include "Math/BoundingBox.h"
#include "Math/Sphere.h"
#include "Math/Matrix.h"
#include "Math/Quaternion.h"
#include "Math/Frustum.h"
#include "Math/Plane.h"
#include "Math/MathUtil.h"