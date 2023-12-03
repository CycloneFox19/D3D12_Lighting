#include <Camera.h>

namespace
{
	float Cos(float rad)
	{
		if (abs(rad) < FLT_EPSILON)
		{
			return 1.0f;
		}

		return cosf(rad);
	}

	float Sin(float rad)
	{
		if (abs(rad) < FLT_EPSILON)
		{
			return 0.0f;
		}

		return sinf(rad);
	}

	// find angle
	float CalcAngle(float sine, float cosine)
	{
		auto result = asinf(sine);
		if (cosine < 0.0f)
		{
			result = DirectX::XM_PI - result;
		}

		return result;
	}

	// find angle and distance from particular vector
	void ToAngle
	(
		const DirectX::SimpleMath::Vector3& v,
		float* angleH,
		float* angleV,
		float* dist
	)
	{
		if (dist != nullptr)
		{
			*dist = v.Length();
		}

		DirectX::SimpleMath::Vector3 src(-v.x, 0.0f, -v.z);
		DirectX::SimpleMath::Vector3 dst = src;

		if (angleH != nullptr)
		{
			// normalize
			if (fabs(src.x) > FLT_EPSILON || fabs(src.z) > FLT_EPSILON)
			{
				src.Normalize(dst);
			}

			*angleH = CalcAngle(dst.x, dst.z); // angle from z axis
		}

		if (angleV != nullptr)
		{
			auto d = src.Length();
			src.x = d;
			src.y = -v.y;
			src.z = 0.0f;

			dst = src;

			// normalize
			if (fabs(src.x) > FLT_EPSILON || fabs(src.y) > FLT_EPSILON)
			{
				src.Normalize(dst);
			}

			*angleV = CalcAngle(dst.y, dst.x); // angle from x(horizontal) axis
		}
	}

	// find forward vector and upward vector from particular angles
	void ToVector
	(
		float angleH,
		float angleV,
		DirectX::SimpleMath::Vector3* forward,
		DirectX::SimpleMath::Vector3* upward
	)
	{
		auto sx = Sin(angleH);
		auto cx = Cos(angleH);
		auto sy = Sin(angleV);
		auto cy = Cos(angleV);

		if (forward != nullptr)
		{
			forward->x = -cy * sx;
			forward->y = -sy;
			forward->z = -cy * cx;
		}

		if (upward != nullptr)
		{
			upward->x = -sy * sx;
			upward->y = cy;
			upward->z = -sy * cx;
		}
	}
} // namespace

//
// Camera class
//

// constructor
Camera::Camera()
{
	m_Current.Position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, -1.0f);
	m_Current.Target = DirectX::SimpleMath::Vector3::Zero;
	m_Current.Upward = DirectX::SimpleMath::Vector3::UnitY;
	m_Current.Angle = DirectX::XMFLOAT2(0.0f, 0.0f);
	m_Current.Forward = DirectX::SimpleMath::Vector3::UnitZ;
	m_Current.Distance = 1.0f;
	m_DirtyFlag = DirtyPosition;

	m_Preserve = m_Current;
}

// destructor
Camera::~Camera()
{
}

// set position of camera
void Camera::SetPosition(const DirectX::SimpleMath::Vector3& value)
{
	m_Current.Position = value;
	ComputeTarget();
	m_DirtyFlag = DirtyAngle;
	Update();
}


// set target of camera
void Camera::SetTarget(const DirectX::SimpleMath::Vector3& value)
{
	m_Current.Target = value;
	ComputePosition();
	m_DirtyFlag = DirtyAngle;
	Update();
}

// update based on camera event
void Camera::UpdateByEvent(const Event& value)
{
	if (value.Type & EventRotate)
	{
		Rotate(value.RotateH, value.RotateV);
	}

	if (value.Type & EventPanTilt)
	{
		Pantilt(value.Pan, value.Tilt);
	}

	if (value.Type & EventDolly)
	{
		Dolly(value.Dolly);
	}

	if (value.Type & EventMove)
	{
		Move(value.MoveX, value.MoveY, value.MoveZ);
	}

	if (value.Type & EventReset)
	{
		Reset();
		return;
	}

	Update();
}

// update
void Camera::Update()
{
	if (m_DirtyFlag == DirtyNone)
	{
		return;
	}

	if (m_DirtyFlag & DirtyPosition)
	{
		ComputePosition();
	}

	if (m_DirtyFlag & DirtyTarget)
	{
		ComputeTarget();
	}

	if (m_DirtyFlag & DirtyAngle)
	{
		ComputeAngle();
	}

	m_View = DirectX::SimpleMath::Matrix::CreateLookAt(
		m_Current.Position,
		m_Current.Target,
		m_Current.Upward);

	m_DirtyFlag = DirtyNone;
}

// preserve current camera parameters
void Camera::Preserve()
{
	m_Preserve = m_Current;
}

// reset camera parameters
void Camera::Reset()
{
	m_Current = m_Preserve;
	m_DirtyFlag = DirtyMatrix;
	Update();
}

// get horizontal rotation angle of camera
const float& Camera::GetAngleH() const
{
	return m_Current.Angle.x;
}

// get vertical rotation angle of camera
const float& Camera::GetAngleV() const
{
	return m_Current.Angle.y;
}

// get distance from camera to target
const float& Camera::GetDistance() const
{
	return m_Current.Distance;
}

// get camera position
const DirectX::SimpleMath::Vector3& Camera::GetPosition() const
{
	return m_Current.Position;
}

// get target of camera
const DirectX::SimpleMath::Vector3& Camera::GetTarget() const
{
	return m_Current.Target;
}

// get upward vector of camera
const DirectX::SimpleMath::Vector3& Camera::GetUpward() const
{
	return m_Current.Upward;
}

// get view matrix
const DirectX::SimpleMath::Matrix& Camera::GetView() const
{
	return m_View;
}

// rotate around the target
void Camera::Rotate(float angleH, float angleV)
{
	ComputeAngle();
	ComputeTarget();

	m_Current.Angle.x += angleH;
	m_Current.Angle.y += angleV;

	// prevention of gimbal lock
	{
		if (m_Current.Angle.y > DirectX::XM_PIDIV2 - FLT_EPSILON)
		{
			m_Current.Angle.y = DirectX::XM_PIDIV2 - FLT_EPSILON;
		}

		if (m_Current.Angle.y < -DirectX::XM_PIDIV2 + FLT_EPSILON)
		{
			m_Current.Angle.y = -DirectX::XM_PIDIV2 + FLT_EPSILON;
		}
	}

	m_DirtyFlag |= DirtyPosition;
}

// pantilt
void Camera::Pantilt(float angleH, float angleV)
{
	ComputeAngle();
	ComputePosition();

	m_Current.Angle.x += angleH;
	m_Current.Angle.y += angleV;

	m_DirtyFlag |= DirtyTarget;
}

// move on base vector of view space
void Camera::Move(float moveX, float moveY, float moveZ)
{
	auto translate = m_View.Right() * moveX + m_View.Up() * moveY + m_View.Forward() * moveZ;

	m_Current.Position += translate;
	m_Current.Target += translate;

	m_DirtyFlag |= DirtyMatrix;
}

// dolly
void Camera::Dolly(float value)
{
	ComputeAngle();
	ComputeTarget();

	m_Current.Distance += value;
	if (m_Current.Distance < 0.001f)
	{
		m_Current.Distance = 0.001f;
	}

	m_DirtyFlag |= DirtyPosition;
}

// calculate camera position
void Camera::ComputePosition()
{
	ToVector(m_Current.Angle.x, m_Current.Angle.y, &m_Current.Forward, &m_Current.Upward);
	m_Current.Position = m_Current.Target - m_Current.Distance * m_Current.Forward;
}

// calculate target
void Camera::ComputeTarget()
{
	ToVector(m_Current.Angle.x, m_Current.Angle.y, &m_Current.Forward, &m_Current.Upward);
	m_Current.Target = m_Current.Position + m_Current.Distance * m_Current.Forward;
}

// calculate rotation angle
void Camera::ComputeAngle()
{
	m_Current.Forward = m_Current.Target - m_Current.Position;
	ToAngle(m_Current.Forward, &m_Current.Angle.x, &m_Current.Angle.y, &m_Current.Distance);
	ToVector(m_Current.Angle.x, m_Current.Angle.y, nullptr, &m_Current.Upward);
}

//
// Projector class
//

// constructor
Projector::Projector()
{
	m_Current.Mode = Mode::Perspective;
	m_Current.Aspect = 1.333f;
	m_Current.FieldOfView = DirectX::XM_PIDIV4;
	m_Current.NearClip = 1.0f;
	m_Current.FarClip = 1000.0f;
	m_Current.Left = 0.0f;
	m_Current.Right = 100.0f;
	m_Current.Top = 0.0f;
	m_Current.Bottom = 100.0f;

	m_Preserve = m_Current;
}

// destructor
Projector::~Projector()
{
}

// preserve current projection parameter
void Projector::Preserve()
{
	m_Preserve = m_Current;
}

// reset projection parameter
void Projector::Reset()
{
	m_Current = m_Preserve;
	switch (m_Current.Mode)
	{
	case Mode::Perspective:
	{
		m_Proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
			m_Current.FieldOfView,
			m_Current.Aspect,
			m_Current.NearClip,
			m_Current.FarClip);
	}
	break;

	case Mode::Orthographic:
	{
		m_Proj = DirectX::SimpleMath::Matrix::CreateOrthographicOffCenter(
			m_Current.Left,
			m_Current.Right,
			m_Current.Bottom,
			m_Current.Top,
			m_Current.NearClip,
			m_Current.FarClip);
	}
	break;
	}
}

// set perspective projection parameters
void Projector::SetPerspective(float fov, float aspect, float nearClip, float farClip)
{
	m_Current.Mode = Mode::Perspective;
	m_Current.FieldOfView = fov;
	m_Current.Aspect = aspect;
	m_Current.NearClip = nearClip;
	m_Current.FarClip = farClip;

	m_Proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(fov, aspect, nearClip, farClip);
}

// set orthographic projection parameters
void Projector::SetOrthographic(float left, float right, float top, float bottom, float nearClip, float farClip)
{
	m_Current.Mode = Mode::Orthographic;
	m_Current.Left = left;
	m_Current.Right = right;
	m_Current.Top = top;
	m_Current.Bottom = bottom;

	m_Proj = DirectX::SimpleMath::Matrix::CreateOrthographicOffCenter(left, right, bottom, top, nearClip, farClip);
}

// get projection mode
const Projector::Mode& Projector::GetMode() const
{
	return m_Current.Mode;
}

// get field of view
const float& Projector::GetFieldOfView() const
{
	return m_Current.FieldOfView;
}

// get aspect
const float& Projector::GetAspect() const
{
	return m_Current.Aspect;
}

// get distance to the near clip plane
const float& Projector::GetNearClip() const
{
	return m_Current.NearClip;
}

// get distance to the far clip plane
const float& Projector::GetFarClip() const
{
	return m_Current.FarClip;
}

// get projection matrix
const DirectX::SimpleMath::Matrix& Projector::GetMatrix() const
{
	return m_Proj;
}
