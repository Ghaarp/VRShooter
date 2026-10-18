#include "Common/Structs.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

FSocketTransformLocal::FSocketTransformLocal(USkeletalMeshComponent* Mesh, FName SocketName)
{

	if (!Mesh)
		return;

	USkeletalMesh* SMesh = Mesh->SkeletalMesh;
	if (!SMesh)
		return;

	USkeletalMeshSocket* Socket = SMesh->FindSocket(SocketName);
	if (!Socket)
		return;

	bIsSocketFound = true;
	SocketTransform = Socket->GetSocketTransform(Mesh);
}

FSocketTransformWorld::FSocketTransformWorld(USkeletalMeshComponent* Mesh, FName SocketName)
{

}