#pragma once

// Includes
#include <cstdint>
#include <d3d12.h>
#include <SimpleMath.h>

//
// Camera class
//
class Camera
{

public:
	//
	// EventType enum
	//
	enum EventType
	{
		EventRotate = 0x1 << 0, //!< rotation
		EventDolly = 0x1 << 1, //!< zoom
		EventMove = 0x1 << 2, //!< move
		EventPanTilt = 0x1 << 3, //!< pan, tilt
		EventReset = 0x1 << 4, //!< reset
	};

	//
	// Event structure
	//
	struct Event
	{
		uint32_t Type = 0; //!< type of event
		float RotateH = 0.0f; //!< horizontal rotation angle(rad)
		float RotateV = 0.0f; //!< vertical rotation angle(rad)
		float Pan = 0.0f; //!< L-R swing angle(rad)
		float Tilt = 0.0f; //!< Up-Down swing angle(rad)
		float Dolly = 0.0f; //!< zoom value
		float MoveX = 0.0f; //!< movement value in x axis(view space)
		float MoveY = 0.0f; //!< movement value in y axis(view space)
		float MoveZ = 0.0f; //!< movement value in z axis(view space)
	};

	// public methods

	Camera();
	~Camera();
	void SetPosition(const DirectX::SimpleMath::Vector3& value);
	void SetTarget(const DirectX::SimpleMath::Vector3& value);
	void UpdateByEvent(const Event& value);
	void Update();
	void Preserve();
	void Reset();

	const float& GetAngleV() const;
	const float& GetAngleH() const;
	const float& GetDistance() const;

	const DirectX::SimpleMath::Vector3& GetPosition() const;
	const DirectX::SimpleMath::Vector3& GetTarget() const;
	const DirectX::SimpleMath::Vector3& GetUpward() const;
	const DirectX::SimpleMath::Matrix& GetView() const;

private:
	//
	// DirtyFlag enum
	//
	enum DirtyFlag
	{
		DirtyNone = 0x0, //!< no re-calculation
		DirtyPosition = 0x1 << 0, //!< re-calculate position
		DirtyTarget = 0x1 << 1, //!< re-calculate target point
		DirtyAngle = 0x1 << 2, //!< re-calculate rotation angle
		DirtyMatrix = 0x1 << 3, //!< re-calculate matrix
	};

	//
	// Param structure
	//
	struct Param
	{
		DirectX::SimpleMath::Vector3 Position;
		DirectX::SimpleMath::Vector3 Target;
		DirectX::SimpleMath::Vector3 Upward;
		DirectX::SimpleMath::Vector3 Forward;
		DirectX::XMFLOAT2 Angle;
		float Distance;
	};

	// private variables
	Param m_Current = {};
	Param m_Preserve = {};
	DirectX::SimpleMath::Matrix m_View = DirectX::SimpleMath::Matrix::Identity;
	uint32_t m_DirtyFlag = 0;

	// private methods
	void Rotate(float angleH, float angleV);
	void Pantilt(float angleH, float angleV);
	void Move(float moveX, float moveY, float moveZ);
	void Dolly(float value);

	void ComputePosition();
	void ComputeTarget();
	void ComputeAngle();
};

//
// Projector class
//
class Projector
{

public:
	//
	// Mode enum
	//
	enum Mode
	{
		Perspective, //!< Perspective Projection
		Orthographic, //!< Orthographic Projection
	};

	// public methods
	Projector();
	~Projector();

	void Preserve();
	void Reset();

	void SetPerspective(float fov, float aspect, float nearClip, float farClip);
	void SetOrthographic(float left, float right, float top, float bottom, float nearClip, float farClip);

	const Mode& GetMode() const;
	const float& GetFieldOfView() const;
	const float& GetAspect() const;
	const float& GetNearClip() const;
	const float& GetFarClip() const;


	const DirectX::SimpleMath::Matrix& GetMatrix() const;

private:

	// private variables
	struct Param
	{
		Mode Mode; //!< Projection mode
		float Aspect; //!< Aspect ratio
		float FieldOfView; //!< Angle of view
		float Left; //!< Left edge
		float Right; //!< Right edge
		float Top; //!< Upper edge
		float Bottom; //!< Lower edge
		float NearClip; //!< Distance to the near clip plane
		float FarClip; //!< Distance to the far clip plane
	};
	Param m_Current;
	Param m_Preserve;

	DirectX::SimpleMath::Matrix m_Proj; //!< Projection matrix
};
