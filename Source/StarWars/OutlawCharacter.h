// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "OutlawCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UENUM(BlueprintType)
enum class EHitboxType : uint8
{
	Capsule,
	Box
};

/**
 * 콤보 1단계 데이터
 * 에디터(BP_OutlawCharacter) Details 패널에서 몽타주와 수치를 직접 세팅
 */
USTRUCT(BlueprintType)
struct FComboStepData
{
	GENERATED_BODY()

	/** 재생할 애니메이션 몽타주 — 없으면 타이머로 모션 시뮬레이션 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Animation")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	/** 다음 콤보 입력 수용 창 시작 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Timing", meta = (ClampMin = "0.0"))
	float ComboWindowOpenTime = 0.533f;

	/** 다음 콤보 입력 수용 창 종료 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Timing", meta = (ClampMin = "0.0"))
	float ComboWindowCloseTime = 0.833f;

	/** 공격력에 곱해질 데미지 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Damage", meta = (ClampMin = "0.0"))
	float DamageRatio = 0.8f;

	/** 타격 시 MaxBlasterEnergy 대비 충전 비율 (0.0 ~ 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EnergyChargeRatio = 0.10f;

	/** 적 경직 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Damage", meta = (ClampMin = "0.0"))
	float HitStunDuration = 0.10f;

	/** N3 파워 슬래시만 true — 넉백 발생 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Damage")
	bool bHasKnockback = false;

	/** 히트박스 형태 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Hitbox")
	EHitboxType HitboxType = EHitboxType::Capsule;

	/**
	 * 히트박스 크기
	 * Capsule: X = 반지름, Y = 절반 높이
	 * Box:     XYZ = 절반 크기(Half Extents)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Hitbox")
	FVector HitboxSize = FVector(40.f, 75.f, 0.f);

	/** 캐릭터 로컬 좌표 기준 히트박스 중심 오프셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo|Hitbox")
	FVector HitboxOffset = FVector(60.f, 0.f, 50.f);
};

/**
 * 아웃로우 주인공 캐릭터
 * Phase 2: 3단 광선검 콤보 + 히트 판정 + 분수식 데미지 계산
 */
UCLASS()
class STARWARS_API AOutlawCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOutlawCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// =========================================================
	// 카메라
	// =========================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComp;

	// =========================================================
	// Enhanced Input
	// =========================================================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MeleeAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RangedAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ForceExecutionAction;

	// =========================================================
	// 전투 자원
	// =========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Resources")
	float BlasterEnergy = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Resources")
	float MaxBlasterEnergy = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Resources")
	float BlasterEnergyCostPerShot = 25.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|Resources")
	bool bCanUseForce = false;

	// =========================================================
	// 데미지 계산 파라미터 (분수식 감면 모델)
	//
	// 기본 데미지  = BasePower × WeaponMultiplier × DamageRatio
	// 방어 감면율  = DefenseConst / (DefenseConst + 적 방어력)
	// 최종 데미지  = 기본 데미지 × 방어 감면율 × 치명타 배율 × 속성 배율
	// =========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	float BasePower = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	float WeaponMultiplier = 1.0f;

	/** 방어 감면 상수 — 높을수록 방어력 영향이 줄어듦 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Damage")
	float DefenseConst = 100.f;

	// =========================================================
	// 콤보 시스템
	// =========================================================
	/** 3단 콤보 데이터 (에디터에서 몽타주 할당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	TArray<FComboStepData> ComboSteps;

	/** 현재 콤보 인덱스 (-1 = 비전투) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	int32 CurrentComboIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	bool bIsAttacking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	bool bInComboWindow = false;

public:
	// =========================================================
	// 히트 판정 — ANS_MeleeAttack에서 호출
	// =========================================================
	void StartMeleeTrace();
	void PerformMeleeTrace();
	void EndMeleeTrace();

	/** 데미지 계산 (분수식 감면 모델) — CritMultiplier/AttributeMultiplier는 Phase3 구현 예정 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Damage")
	float CalculateFinalDamage(int32 ComboIndex, float TargetDefense, float CritMultiplier = 1.f, float AttributeMultiplier = 1.f) const;

private:
	bool bComboInputBuffered = false;

	FTimerHandle ComboWindowOpenTimer;
	FTimerHandle ComboWindowCloseTimer;
	FTimerHandle ComboResetTimer;

	/** 이전 프레임의 트레이스 원점 (Sweep 궤적 계산용) */
	FVector LastWeaponTraceOrigin = FVector::ZeroVector;

	/** 한 콤보 단계에서 이미 맞은 액터 목록 (중복 히트 방지) */
	TArray<TWeakObjectPtr<AActor>> CurrentComboHitActors;

	void StartCombo(int32 Index);
	void OnComboWindowOpen();
	void OnComboWindowClose();
	void ResetCombo();

	void OnMeleeHit(const FHitResult& Hit);

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleMeleeAttack(const FInputActionValue& Value);
	void HandleRangedAttack(const FInputActionValue& Value);
	void HandleForceExecution(const FInputActionValue& Value);

public:
	virtual void Tick(float DeltaTime) override;
};
