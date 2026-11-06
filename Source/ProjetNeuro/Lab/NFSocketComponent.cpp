// Neuron Forge - ENSIM

#include "Lab/NFSocketComponent.h"
#include "Lab/NFPiece.h"

bool UNFSocketComponent::CanAccept(const ANFPiece* Candidate) const
{
	return Candidate
		&& IsFree()
		&& Candidate->PieceType == AcceptedType
		&& GetOwner() != Candidate;	// une pièce ne s'emboîte pas dans elle-même
}
