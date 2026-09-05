export module Runtime.RDG:Validation;

import Core;
import :Pass;

export namespace Vortex {

void validateRDGName(std::string_view name, std::string_view kind);
void validateRDGAccessMode(const RDGPassNode& pass, const RDGResource& resource, const RDGResourceAccess& access);
void validateRDGTextureAccess(const RDGPassNode& pass, const RDGTexture& texture, const RDGResourceAccess& access);
void validateRDGBufferAccess(const RDGPassNode& pass, const RDGBuffer& buffer, const RDGResourceAccess& access);
void validateRDGResourceAccess(const RDGGraph& graph, const RDGPassNode& pass, const RDGResourceAccess& access);
void validateRDGRasterPass(const RDGPassNode& pass);
void validateRDGCopyPass(const RDGPassNode& pass);
void validateRDGPass(const RDGGraph& graph, RDGPassHandle pass);
void validateRDGGraph(const RDGGraph& graph);

}        // namespace Vortex
