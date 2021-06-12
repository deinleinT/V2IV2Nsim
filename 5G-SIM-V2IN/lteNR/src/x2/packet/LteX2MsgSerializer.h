//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef X2_PACKET_LTEX2MSGSERIALIZER_H_
#define X2_PACKET_LTEX2MSGSERIALIZER_H_

#include "stack/compManager/compManagerProportional/X2CompProportionalReplyIE.h"
#include "inet/common/packet/serializer/FieldsChunkSerializer.h"

class LteX2MsgSerializer : public inet::FieldsChunkSerializer {
private:
    void serializeStatusMap(inet::MemoryOutputStream& stream, std::vector<CompRbStatus> map) const;
    std::vector<CompRbStatus> deserializeStatusMap(inet::MemoryInputStream& stream) const;
protected:
  virtual void serialize(inet::MemoryOutputStream& stream, const inet::Ptr<const inet::Chunk>& chunk) const override;
  virtual const inet::Ptr<inet::Chunk> deserialize(inet::MemoryInputStream& stream) const override;

public:
  LteX2MsgSerializer() : FieldsChunkSerializer() {}
};

#endif /* X2_PACKET_LTEX2MSGSERIALIZER_H_ */

