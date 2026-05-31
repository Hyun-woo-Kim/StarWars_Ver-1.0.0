// Copyright Epic Games, Inc. All Rights Reserved.

#include "OutlawCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Animation/AnimInstance.h"

AOutlawCharacter::AOutlawCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// ---------------------------------------------------------
	// 스프링 암 & 카메라 (기획서 Phase 1 수치)
	// ---------------------------------------------------------
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength         = 400.f;
	SpringArmComp->SocketOffset            = FVector(0.f, 0.f, 70.f);
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->bEnableCameraLag        = true;
	SpringArmComp->CameraLagSpeed          = 10.f; // 0.1초 지연 ≈ 속도 10

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;
	CameraComp->FieldOfView             = 90.f;

	// ---------------------------------------------------------
	// 이동 설정 (기획서 수치)
	// ---------------------------------------------------------
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	GetCharacterMovement()->bOrientRotationToMovement  = true;
	GetCharacterMovement()->RotationRate               = FRotator(0.f, 600.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed               = 400.f;
	GetCharacterMovement()->JumpZVelocity              = 700.f;
	GetCharacterMovement()->AirControl                 = 0.35f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// ---------------------------------------------------------
	// 콤보 데이터 초기화 (기획서 30fps 기준)
	//
	// 입력 윈도우: N1 16~25F, N2 21~30F (기획서 최신 수치)
	// 히트박스:    N1 Capsule(r40,h150) / N2 Box(80×250×100) / N3 Box(120×100×200)
	// 데미지 배율: N1=0.8, N2=1.0, N3=1.5
	// ---------------------------------------------------------
	ComboSteps.SetNum(3);

	// N1 — 쾌속 베기 (30F = 1.0s)
	ComboSteps[0].DamageRatio          = 0.8f;
	ComboSteps[0].EnergyChargeRatio    = 0.10f;
	ComboSteps[0].HitStunDuration      = 0.10f;
	ComboSteps[0].bHasKnockback        = false;
	ComboSteps[0].ComboWindowOpenTime  = 16.f / 30.f;   // 0.533s
	ComboSteps[0].ComboWindowCloseTime = 25.f / 30.f;   // 0.833s
	ComboSteps[0].HitboxType           = EHitboxType::Capsule;
	ComboSteps[0].HitboxSize           = FVector(40.f, 75.f, 0.f);    // r=40, halfH=75
	ComboSteps[0].HitboxOffset         = FVector(60.f, 0.f, 50.f);

	// N2 — 횡 베기 (35F = 1.167s)
	ComboSteps[1].DamageRatio          = 1.0f;
	ComboSteps[1].EnergyChargeRatio    = 0.15f;
	ComboSteps[1].HitStunDuration      = 0.15f;
	ComboSteps[1].bHasKnockback        = false;
	ComboSteps[1].ComboWindowOpenTime  = 21.f / 35.f;   // 0.600s
	ComboSteps[1].ComboWindowCloseTime = 30.f / 35.f;   // 0.857s
	ComboSteps[1].HitboxType           = EHitboxType::Box;
	ComboSteps[1].HitboxSize           = FVector(40.f, 125.f, 50.f);  // half-extents of 80×250×100
	ComboSteps[1].HitboxOffset         = FVector(100.f, 0.f, 50.f);

	// N3 — 파워 슬래시 (40F = 1.333s, 콤보 종결)
	ComboSteps[2].DamageRatio          = 1.5f;
	ComboSteps[2].EnergyChargeRatio    = 0.25f;
	ComboSteps[2].HitStunDuration      = 0.30f;
	ComboSteps[2].bHasKnockback        = true;
	ComboSteps[2].ComboWindowOpenTime  = 0.f;            // 다음 콤보 없음
	ComboSteps[2].ComboWindowCloseTime = 0.f;
	ComboSteps[2].HitboxType           = EHitboxType::Box;
	ComboSteps[2].HitboxSize           = FVector(60.f, 50.f, 100.f);  // half-extents of 120×100×200
	ComboSteps[2].HitboxOffset         = FVector(150.f, 0.f, 70.f);
}

void AOutlawCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AOutlawCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction,           ETriggerEvent::Triggered, this, &AOutlawCharacter::HandleMove);
		EIC->BindAction(LookAction,           ETriggerEvent::Triggered, this, &AOutlawCharacter::HandleLook);
		EIC->BindAction(MeleeAttackAction,    ETriggerEvent::Started,   this, &AOutlawCharacter::HandleMeleeAttack);
		EIC->BindAction(RangedAttackAction,   ETriggerEvent::Started,   this, &AOutlawCharacter::HandleRangedAttack);
		EIC->BindAction(ForceExecutionAction, ETriggerEvent::Started,   this, &AOutlawCharacter::HandleForceExecution);
	}
}

// =========================================================
// 이동 & 시점
// =========================================================
void AOutlawCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MV = Value.Get<FVector2D>();
	if (!Controller) return;

	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), MV.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), MV.X);
}

void AOutlawCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LV = Value.Get<FVector2D>();
	if (!Controller) return;
	AddControllerYawInput(LV.X);
	AddControllerPitchInput(LV.Y);
}

// =========================================================
// 광선검 기본 공격
// =========================================================
void AOutlawCharacter::HandleMeleeAttack(const FInputActionValue& Value)
{
	if (!bIsAttacking)
	{
		StartCombo(0);
	}
	else if (bInComboWindow)
	{
		bComboInputBuffered = true;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(4, 1.f, FColor::Yellow,
				FString::Printf(TEXT("[콤보 버퍼] N%d 입력 예약"), CurrentComboIndex + 2));
		}
	}
}

// =========================================================
// 블래스터 발사
// =========================================================
void AOutlawCharacter::HandleRangedAttack(const FInputActionValue& Value)
{
	if (BlasterEnergy < BlasterEnergyCostPerShot)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Red,
				TEXT("[블래스터] 에너지 부족! 광선검으로 충전하세요."));
		}
		return;
	}

	BlasterEnergy = FMath::Clamp(BlasterEnergy - BlasterEnergyCostPerShot, 0.f, MaxBlasterEnergy);

	// TODO: 발사체 스폰
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 2.f, FColor::Green,
			FString::Printf(TEXT("[블래스터] 발사! 남은 에너지: %.0f / %.0f"), BlasterEnergy, MaxBlasterEnergy));
	}
}

// =========================================================
// 포스 처형
// =========================================================
void AOutlawCharacter::HandleForceExecution(const FInputActionValue& Value)
{
	if (!bCanUseForce)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(3, 2.f, FColor::Orange,
				TEXT("[포스 처형] 그로기 상태의 적이 없습니다."));
		}
		return;
	}

	bCanUseForce  = false;
	BlasterEnergy = MaxBlasterEnergy;

	// TODO: 처형 애니메이션 재생
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3, 3.f, FColor::Purple,
			TEXT("[포스 처형] 성공! 블래스터 에너지 완전 회복!"));
	}
}

// =========================================================
// 콤보 시스템
// =========================================================
void AOutlawCharacter::StartCombo(int32 Index)
{
	if (!ComboSteps.IsValidIndex(Index)) return;

	const FComboStepData& Step = ComboSteps[Index];

	bIsAttacking        = true;
	bInComboWindow      = false;
	bComboInputBuffered = false;
	CurrentComboIndex   = Index;

	GetWorldTimerManager().ClearTimer(ComboWindowOpenTimer);
	GetWorldTimerManager().ClearTimer(ComboWindowCloseTimer);
	GetWorldTimerManager().ClearTimer(ComboResetTimer);

	// 각 섹션 이름 (몽타주 에디터의 섹션명과 일치해야 함)
	static const FName SectionNames[] = { TEXT("N1"), TEXT("N2"), TEXT("N3") };

	// 기획서 기준 fallback 모션 시간 (몽타주 없을 때 타이머로 시뮬레이션)
	static const float MotionDurations[] = { 1.0f, 1.167f, 1.333f };

	bool bMontageStarted = false;
	if (ComboMontage)
	{
		if (UAnimInstance* AI = GetMesh()->GetAnimInstance())
		{
			if (Index == 0)
			{
				float Duration = AI->Montage_Play(ComboMontage);
				if (Duration > 0.f)
				{
					bMontageStarted = true;
					FOnMontageEnded EndDelegate;
					EndDelegate.BindUObject(this, &AOutlawCharacter::OnAttackMontageEnded);
					AI->Montage_SetEndDelegate(EndDelegate, ComboMontage);
				}
			}
			else
			{
				AI->Montage_JumpToSection(SectionNames[Index], ComboMontage);
				bMontageStarted = AI->Montage_IsPlaying(ComboMontage);
			}
		}
	}

	if (!bMontageStarted)
	{
		// 몽타주 없을 때 — 타이머로 모션 종료 시뮬레이션
		GetWorldTimerManager().SetTimer(
			ComboResetTimer, this, &AOutlawCharacter::ResetCombo,
			MotionDurations[FMath::Clamp(Index, 0, 2)], false);
	}

	// 콤보 입력 창 타이머 (N3 제외)
	if (Index < ComboSteps.Num() - 1 && Step.ComboWindowOpenTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			ComboWindowOpenTimer, this, &AOutlawCharacter::OnComboWindowOpen,
			Step.ComboWindowOpenTime, false);
	}

	// 디버그
	static const TCHAR* Names[]  = { TEXT("N1 쾌속 베기"), TEXT("N2 횡 베기"), TEXT("N3 파워 슬래시") };
	static const FColor Colors[] = { FColor::Cyan, FColor::Green, FColor::Magenta };

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 2.f, Colors[Index],
			FString::Printf(TEXT("[광선검] %s"), Names[Index]));

		if (Step.bHasKnockback)
		{
			GEngine->AddOnScreenDebugMessage(5, 1.5f, FColor::Orange, TEXT("[넉백 발생!]"));
		}
	}
}

void AOutlawCharacter::OnComboWindowOpen()
{
	bInComboWindow = true;

	const FComboStepData& Step = ComboSteps[CurrentComboIndex];
	const float WindowDuration = Step.ComboWindowCloseTime - Step.ComboWindowOpenTime;

	if (WindowDuration > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			ComboWindowCloseTimer, this, &AOutlawCharacter::OnComboWindowClose,
			WindowDuration, false);
	}
}

void AOutlawCharacter::OnComboWindowClose()
{
	bInComboWindow = false;

	if (bComboInputBuffered)
	{
		const int32 NextIndex = CurrentComboIndex + 1;
		if (ComboSteps.IsValidIndex(NextIndex))
		{
			StartCombo(NextIndex);
			return;
		}
	}
	// 버퍼 없으면 몽타주 종료(또는 ComboResetTimer)까지 대기
}

void AOutlawCharacter::ResetCombo()
{
	GetWorldTimerManager().ClearTimer(ComboWindowOpenTimer);
	GetWorldTimerManager().ClearTimer(ComboWindowCloseTimer);
	GetWorldTimerManager().ClearTimer(ComboResetTimer);

	bIsAttacking        = false;
	bInComboWindow      = false;
	bComboInputBuffered = false;
	CurrentComboIndex   = -1;
}

void AOutlawCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bIsAttacking) return;

	if (Montage == ComboMontage)
	{
		ResetCombo();
	}
}

// =========================================================
// 히트 판정 — ANS_MeleeAttack에서 호출
// =========================================================
void AOutlawCharacter::StartMeleeTrace()
{
	CurrentComboHitActors.Reset();

	// 트레이스 시작 원점 초기화 (현재 위치)
	if (ComboSteps.IsValidIndex(CurrentComboIndex))
	{
		const FVector LocalOffset = ComboSteps[CurrentComboIndex].HitboxOffset;
		LastWeaponTraceOrigin = GetActorTransform().TransformPosition(LocalOffset);
	}
}

void AOutlawCharacter::PerformMeleeTrace()
{
	if (!ComboSteps.IsValidIndex(CurrentComboIndex)) return;

	const FComboStepData& Step = ComboSteps[CurrentComboIndex];
	const FVector CurrentOrigin = GetActorTransform().TransformPosition(Step.HitboxOffset);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	for (const TWeakObjectPtr<AActor>& WeakActor : CurrentComboHitActors)
	{
		if (WeakActor.IsValid())
		{
			Params.AddIgnoredActor(WeakActor.Get());
		}
	}

	TArray<FHitResult> HitResults;
	bool bHit = false;

	if (Step.HitboxType == EHitboxType::Capsule)
	{
		// Capsule: HitboxSize.X = 반지름, HitboxSize.Y = 절반 높이
		bHit = GetWorld()->SweepMultiByChannel(
			HitResults,
			LastWeaponTraceOrigin,
			CurrentOrigin,
			GetActorQuat(),
			ECC_Pawn,
			FCollisionShape::MakeCapsule(Step.HitboxSize.X, Step.HitboxSize.Y),
			Params);
	}
	else
	{
		// Box: HitboxSize = Half Extents
		bHit = GetWorld()->SweepMultiByChannel(
			HitResults,
			LastWeaponTraceOrigin,
			CurrentOrigin,
			GetActorQuat(),
			ECC_Pawn,
			FCollisionShape::MakeBox(Step.HitboxSize),
			Params);
	}

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor != this)
			{
				CurrentComboHitActors.Add(HitActor);
				OnMeleeHit(Hit);
			}
		}
	}

	LastWeaponTraceOrigin = CurrentOrigin;
}

void AOutlawCharacter::EndMeleeTrace()
{
	CurrentComboHitActors.Reset();
}

// =========================================================
// 타격 처리 — 데미지 계산 + 에너지 충전
// =========================================================
float AOutlawCharacter::CalculateFinalDamage(int32 ComboIndex, float TargetDefense,
	float CritMultiplier, float AttributeMultiplier) const
{
	if (!ComboSteps.IsValidIndex(ComboIndex)) return 0.f;

	// [1단계] 기본 데미지
	const float BaseDamage = BasePower * WeaponMultiplier * ComboSteps[ComboIndex].DamageRatio;

	// [2단계] 분수식 방어 감면 — 방어력이 아무리 높아도 데미지가 0이 되지 않음
	const float MitigationRate = DefenseConst / (DefenseConst + FMath::Max(TargetDefense, 0.f));

	// [3단계] 최종 데미지 (치명타·속성 배율은 Phase3 구현 예정, 기본값 1.0)
	return BaseDamage * MitigationRate * CritMultiplier * AttributeMultiplier;
}

void AOutlawCharacter::OnMeleeHit(const FHitResult& Hit)
{
	if (!ComboSteps.IsValidIndex(CurrentComboIndex)) return;

	const FComboStepData& Step = ComboSteps[CurrentComboIndex];

	// 적 방어력 — 나중에 IEnemyInterface 또는 UHealthComponent에서 가져옴
	const float TargetDefense = 0.f;
	const float FinalDamage   = CalculateFinalDamage(CurrentComboIndex, TargetDefense);

	// 블래스터 에너지 충전 (히트 성공 시)
	BlasterEnergy = FMath::Clamp(
		BlasterEnergy + MaxBlasterEnergy * Step.EnergyChargeRatio, 0.f, MaxBlasterEnergy);

	// TODO: UGameplayStatics::ApplyDamage — 적 클래스 구현 후 연결
	// TODO: 히트 스톱 (Phase3)
	// TODO: VFX/SFX 스폰 (Phase3)

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
			FString::Printf(TEXT("[HIT] %s | 데미지: %.0f | 에너지: %.0f / %.0f"),
				*Hit.GetActor()->GetName(), FinalDamage, BlasterEnergy, MaxBlasterEnergy));
	}
}

void AOutlawCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
