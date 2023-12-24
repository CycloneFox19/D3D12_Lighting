#include <IESProfile.h>
#include <vector>
#include <fstream>
#include <Logger.h>
#include <DirectXMath.h>

namespace {
	// constant values
	constexpr int TypeC = 1; // C-Plane
	constexpr int TypeB = 2; // B-Plane
	constexpr int TypeA = 3; // A-Plane

	constexpr int UnitFeet = 1; // unit is feet[ft]
	constexpr int UnitMeter = 2; // unit is meter[m]

	//
	// Lamp structure
	//
	struct Lamp
	{
		float Lumen; //!< luminous flux
		float Multiplier; //!< multiplication coefficient
		int PhotometricType; //!< photometric type(1 : C-Plane, 2 : B-Plane, 3 : A-Plane)
		int UnitType; //!< unit(1 : feet, 2 : meter)
		float ShapeWidth; //!< width of the light shape
		float ShapeLength; //!< length of the light shape
		float ShapeHeight; //!< height of the light
		float BallasFactor; //!< ballast output coefficient
		float InputWatts; //!< input watts
		std::vector<float> AngleV; //!< vertical angle
		std::vector<float> AngleH; //!< horizontal angle
		std::vector<float> Candera; //!< candera
		float  AveCandera; //!< average of candera
	};

	// linear interpolation
	inline float Lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}

	// find index in float decimal // incompleted
	float GetPos(float value, const std::vector<float>& container)
	{
		if (container.size() == 1)
		{
			return container.front();
		}

		if (value < container.front())
		{
			return -1.0f;
		}
	}

	// load IES profile
	bool LoadIESProfile(const char* path, Lamp& lamp)
	{
		std::ifstream stream;

		stream.open(path, std::ios::in);

		if (!stream.is_open())
		{
			ELOG("Error : File Open Failed. path = %s", path);
			return false;
		}

		const std::streamsize BufferSize = 2048;
		char buf[BufferSize] = {};

		stream >> buf;

		// check if it is string to be checked
		if (0 != strcmp(buf, "IESNA:LM-63-2002") &&
			0 != strcmp(buf, "IESNA:LM-63-1995")) // if retval is 0, then the strings are equal.
		{
			ELOG("Error : Invalid IES Profile.");
			stream.close();
			return false;
		}

		for (;;)
		{
			if (!stream)
			{
				break;
			}

			if (stream.eof()) // eof stands for "end of file."
			{
				break;
			}

			stream >> buf;

			// if tilt angle information appeared, analyze it
			if (0 == strcmp(buf, "TILT_NONE"))
			{
				// skip the row starts with "TILT"
				stream.ignore(BufferSize, '\n');

				// get lamp count
				int lampCount = 0;
				stream >> lampCount;

				// this program is compatible with single lamp
				if (lampCount != 1)
				{
					ELOG("Error : Lamp count is %d.", lampCount);
					stream.close();
					return false;
				}

				int angleCountV = 0;
				int angleCountH = 0;
				int futureUse = 0;

				stream >> lamp.Lumen; // lumen
				stream >> lamp.Multiplier; // sort of light intensity
				stream >> angleCountV; // number of vertical angles
				stream >> angleCountH; // number of horizontal angles

				// reserve memory
				lamp.AngleV.reserve(angleCountV);
				lamp.AngleH.reserve(angleCountH);
				lamp.Candera.reserve(angleCountV * angleCountH);

				stream >> lamp.PhotometricType;
				if (lamp.PhotometricType != TypeC)
				{
					ELOG("Error : Out of support.");
					stream.close();
					return false;
				}

				stream >> lamp.UnitType; // unit of the shape of instrument
				stream >> lamp.ShapeWidth; // width of the instrument
				stream >> lamp.ShapeHeight; // height of the instrument
				stream >> lamp.BallasFactor; // ballas output coefficient
				stream >> futureUse; // reserved area
				stream >> lamp.InputWatts; // input watts

				// skip by the change of line
				stream.ignore(BufferSize, '\n');

				float value = 0.0f;

				// angle count of vertical angle
				for (auto i = 0; i < angleCountV; ++i)
				{
					stream >> value;
					lamp.AngleV.push_back(value);
				}

				// angle count of horizontal angle
				for (auto i = 0; i < angleCountH; ++i)
				{
					stream >> value;
					lamp.AngleH.push_back(value);
				}

				// initialize average
				lamp.AveCandera = 0.0f;
				auto count = 0;

				// intensity data
				for (auto i = 0; i < angleCountH; ++i)
				{
					for (auto j = 0; j < angleCountV; ++j)
					{
						stream >> value;
						auto candera = value * lamp.Multiplier;
						lamp.Candera.push_back(candera);
						lamp.AveCandera += candera;
						count++;
					}
				}

				lamp.AveCandera /= float(count);
			}

			// skip by changing
			stream.ignore(BufferSize, '\n');
		}

		// close stream
		stream.close();

		// normal end
		return true;
	}

	// get candera value
	float GetCandera(int x, int y, const Lamp& lamp)
	{
		auto w = int(lamp.AngleV.size()); // on the list, width corresponds to vertical angle count
		auto h = int(lamp.AngleH.size());

		x %= w;
		y %= h;

		auto idx = (w - 1) * y + x;
		assert(idx < lamp.Candera.size());

		return lamp.Candera[idx];
	}

	// bilinear sampling
	float BilinearSample(float x, float y, const Lamp& lamp)
	{
		auto x0 = int(floor(x));
		auto y0 = int(floor(y));

		auto x1 = x0 + 1;
		auto y1 = y0 + 1;

		return (x1 - x) * ((y1 - y) * GetCandera(x0, y0, lamp) + (y - y0) * GetCandera(x0, y1, lamp))
			+ (x - x0) * ((y1 - y) * GetCandera(x1, y0, lamp) + (y - y0) * GetCandera(x1, y1, lamp));
	}

	// get interpolated candera value
	float Interpolate(float angleV, float angleH, const Lamp& lamp)
	{
		// check in maximum range
		assert(0 <= angleV && angleV <= 180.0f);
		assert(0 <= angleH && angleH <= 360.0f);

		// get index in float decimal
		auto s = GetPos(angleV, lamp.AngleV);
		auto t = GetPos(angleH, lamp.AngleH);

		if (s < 0.0f || t < 0.0f)
		{
			return 0.0f;
		}

		// bilinear sampling
		return BilinearSample(s, t, lamp);
	}

} // namespace

//
// IESProfile class
//

// constructor
IESProfile::IESProfile()
	: m_pHandle(nullptr)
	, m_pPool(nullptr)
	, m_Lumen(0.0f)
{
}

// destructor
IESProfile::~IESProfile()
{
	Term();
}

// initialize
bool IESProfile::Init
(
	ID3D12Device* pDevice,
	DescriptorPool* pPool,
	const char* filePath,
	DirectX::ResourceUploadBatch& batch
)
{
	if (pDevice == nullptr || pPool == nullptr || filePath == nullptr)
	{
		ELOG("Error : Invalid Argument.");
		return false;
	}

	Lamp lamp;
	if (!LoadIESProfile(filePath, lamp))
	{
		ELOG("Error : IES Profile Load Failed.");
		return false;
	}

	auto size = 128;
	size = DirectX::XMMax(size, int(lamp.AngleV.size()));
	size = DirectX::XMMax(size, int(lamp.AngleH.size()));

	auto find = false;

	// find the closest power of two in range from 128px to maximum
	for (int i = 7; i <= D3D12_MAX_TEXTURE_DIMENSION_2_TO_EXP; ++i)
	{
		auto lhs = exp2(i - 1);
		auto rhs = exp2(i);

		// check whether it is in range
		if (lhs < size && size <= rhs)
		{
			size = int(rhs);
			find = true;
			break;
		}
	}

	// outcome is error with the size that is out of supported range
	if (!find)
	{
		ELOG("Error : Out of support.");
		return false;
	}

	auto w = size;
	auto h = size;

	m_Candera.clear();
	m_Candera.shrink_to_fit();
	m_Candera.reserve(w * h);

	auto invW = 1.0f / float(w - 1);
	auto invH = 1.0f / float(h - 1);
	auto invA = 1.0f / lamp.AveCandera;

	auto lastH = lamp.AngleH.back();
	auto lastV = int(lamp.AngleV.back());

	for (auto j = 0; j < h; ++j)
	{
		// phi is in [0, 2PI]
		auto angleH = 0.0f;

		if (lastH > 0.0f)
		{
			angleH = j * invH * 360.0f;
			angleH = fmod(angleH, 2.0f * lastH);
			if (angleH > lastH)
			{
				angleH = lastH * 2.0f - angleH;
			}
		}


		for (auto i = 0; i < w; ++i)
		{
			// theta is in range of [0, PI]
			auto rad = i * invW * 2.0f - 1.0f;
			auto angleV = DirectX::XMConvertToDegrees(acos(rad));

			auto cd = invA * Interpolate(angleV, angleH, lamp);

			// contain data
			m_Candera.push_back(cd);
		}
	}

	m_Lumen = lamp.Lumen;

	// generate texture
	m_pPool = pPool;
	m_pPool->AddRef();

	m_pHandle = pPool->AllocHandle();
	if (m_pHandle == nullptr)
	{
		ELOG("Error : DescriptorHandle Allocate Failed.");
		return false;
	}

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = w;
	desc.Height = h;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.CreationNodeMask = 0;
	prop.VisibleNodeMask = 0;

	auto hr = pDevice->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(m_Resource.GetAddressOf()));
	if (FAILED(hr))
	{
		ELOG("Error : ID3D12Device::CreateCommittedResource() Failed. retcode = 0x%x", hr);
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
	viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDesc.Texture2D.MipLevels = 1;
	viewDesc.Texture2D.MostDetailedMip = 0;
	viewDesc.Texture2D.PlaneSlice = 0;
	viewDesc.Texture2D.ResourceMinLODClamp = 0;

	pDevice->CreateShaderResourceView(m_Resource.Get(), &viewDesc, m_pHandle->HandleCPU);

	D3D12_SUBRESOURCE_DATA subRes = {};
	subRes.RowPitch = w * sizeof(float);
	subRes.SlicePitch = h * subRes.RowPitch;
	subRes.pData = m_Candera.data();

	batch.Upload(m_Resource.Get(), 0, &subRes, 1);

	return true;
}

// end
void IESProfile::Term()
{
	m_Resource.Reset();

	if (m_pHandle != nullptr && m_pPool != nullptr)
	{
		m_pPool->FreeHandle(m_pHandle);
		m_pHandle = nullptr;
	}

	if (m_pPool != nullptr)
	{
		m_pPool->Release();
		m_pPool = nullptr;
	}

	m_Candera.clear();
	m_Candera.shrink_to_fit();

	m_Lumen = 0.0f;
}

// get CPU descriptor handle
D3D12_CPU_DESCRIPTOR_HANDLE IESProfile::GetHandleCPU() const
{
	if (m_pHandle != nullptr)
	{
		return m_pHandle->HandleCPU;
	}

	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

// get GPU descriptor handle
D3D12_GPU_DESCRIPTOR_HANDLE IESProfile::GetHandleGPU() const
{
	if (m_pHandle != nullptr)
	{
		return m_pHandle->HandleGPU;
	}

	return D3D12_GPU_DESCRIPTOR_HANDLE();
}

// get resource
ID3D12Resource* IESProfile::GetResource() const
{
	return m_Resource.Get();
}

// get lumen
float IESProfile::GetLumen() const
{
	return m_Lumen;
}