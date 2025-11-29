// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#define JOINT_MAJOR_VERSION	2
#define JOINT_MINOR_VERSION	11
#define JOINT_PATCH_VERSION	2


// Helper for JOINT_VERSION_NEWER_THAN and JOINT_VERSION_OLDER_THAN
#define JOINT_GREATER_SORT(Value, ValueToBeGreaterThan, TieBreaker) \
	(((Value) > (ValueToBeGreaterThan)) || (((Value) == (ValueToBeGreaterThan)) && (TieBreaker)))

// Version comparison macro that is defined to true if the Joint version is newer than MajorVer.MinorVer.PatchVer and false otherwise
// (a typical use is to alert integrators to revisit this code when upgrading to a new engine version)
#define JOINT_VERSION_NEWER_THAN(MajorVersion, MinorVersion, PatchVersion) \
	JOINT_GREATER_SORT(JOINT_MAJOR_VERSION, MajorVersion, JOINT_GREATER_SORT(JOINT_MINOR_VERSION, MinorVersion, JOINT_GREATER_SORT(JOINT_PATCH_VERSION, PatchVersion, false)))

// Version comparison macro that is defined to true if the Joint version is older than MajorVer.MinorVer.PatchVer and false otherwise
// (use when making code that needs to be compatible with older engine versions)
#define JOINT_VERSION_OLDER_THAN(MajorVersion, MinorVersion, PatchVersion) \
	JOINT_GREATER_SORT(MajorVersion, JOINT_MAJOR_VERSION, JOINT_GREATER_SORT(MinorVersion, JOINT_MINOR_VERSION, JOINT_GREATER_SORT(PatchVersion, JOINT_PATCH_VERSION, false)))
