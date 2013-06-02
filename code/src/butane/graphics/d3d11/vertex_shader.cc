// This file is part of Butane. See README.md and LICENSE.md for details.
// Copyright (c) 2012 Michael Williams <devbug@bitbyte.ca>

#include <butane/graphics/d3d11/vertex_shader.h>

#include <butane/application.h>
#include <butane/graphics/d3d11/render_device.h>

namespace butane {
  static Allocator& allocator() {
    static ProxyAllocator allocator("vertex shaders", Allocators::heap());
    return allocator;
  }

  D3D11VertexShader::D3D11VertexShader(
    const Resource::Id id
  ) : VertexShader(id)
    , _resource(nullptr)
    , _byte_code(Allocators::heap())
  {
  }

  D3D11VertexShader::~D3D11VertexShader()
  {
    if (_resource)
      _resource->Release();
  }

  VertexShader* VertexShader::load(
    const Resource::Id id,
    const Resource::Stream& stream )
  {
    const LogScope _("D3D11VertexShader::load");

    const MemoryResidentData& mrd =
      *((const MemoryResidentData*)stream.memory_resident_data());

    D3D11VertexShader* vertex_shader =
      make_new(D3D11VertexShader, allocator())(id);

    vertex_shader->_vertex_declaration = mrd.vertex_declaration;
    vertex_shader->_byte_code.resize(mrd.byte_code_len);
    copy((void*)vertex_shader->_byte_code.raw(), (const void*)&mrd.byte_code[0], mrd.byte_code_len);

    D3D11RenderDevice* render_device =
      ((D3D11RenderDevice*)Application::render_device());

    /* vertex_shader->_resource */ {
      const HRESULT hr = render_device->device()->CreateVertexShader(
        (const void*)&mrd.byte_code[0], mrd.byte_code_len, NULL, &vertex_shader->_resource);

      if (FAILED(hr))
        fail("ID3D11Device::CreateVertexShader failed, hr=%#08x", hr);
    }

    return vertex_shader;
  }

  void VertexShader::unload(
    VertexShader* vertex_shader )
  {
    const LogScope _("D3D11VertexShader::unload");

    assert(vertex_shader != nullptr);
    make_delete(D3D11VertexShader, allocator(), (D3D11VertexShader*)vertex_shader);
  }

  bool VertexShader::compile(
    const Resource::Compiler::Source& src,
    const Resource::Compiler::Stream& cs )
  {
    const LogScope _("D3D11VertexShader::compile");

    static const D3D_SHADER_MACRO defines[] = {
      { "VERTEX_SHADER", "1" },
      { "D3D11",         "1" },
      { NULL, NULL },
    };

  #if defined(BUTANE_DEBUG_BUILD) // || defined(BUTANE_DEVELOPMENT_BUILD)
    static const UINT flags =
      D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY |
      D3DCOMPILE_OPTIMIZATION_LEVEL0 |
      D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
  #else
    static const UINT flags =
      D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY |
      D3DCOMPILE_OPTIMIZATION_LEVEL3 |
      D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
  #endif

    size_t shader_len = 0;
    void* shader = File::read_in(cs.source_data(), Allocators::heap(), &shader_len);

    if (!shader)
      return false;

    ID3DBlob* blob = nullptr;
    ID3DBlob* err_blob = nullptr;

    D3DInclude include;
    include.src = src;

    /* blob, err_blob = */ {
      const HRESULT hr = D3DCompile(
        shader, shader_len, src.path,
        &defines[0], &include,
        "vs_main", "vs_4_0",
        flags, 0, &blob, &err_blob);

      Allocators::heap().free(shader);

      if (FAILED(hr)) {
        log("D3DCompile failed, hr=%#08x", hr);
        log("  %s", err_blob->GetBufferPointer());
        err_blob->Release();
        return false;
      }
    }

    ID3D11ShaderReflection* shader_refl = nullptr; {
      const HRESULT hr = D3DReflect(
        blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_ID3D11ShaderReflection, (void**)&shader_refl);

      if (FAILED(hr)) {
        log("D3DReflect failed, hr=%#08x", hr);
        shader_refl->Release();
        blob->Release();
        return false; }
    }

    MemoryResidentData mrd;
    mrd.vertex_declaration = 0;
    mrd.byte_code_len = blob->GetBufferSize();

    /* mrd.vertex_declaration = */ {
      D3D11_SHADER_DESC desc; {
        const HRESULT hr = shader_refl->GetDesc(&desc);
        if (FAILED(hr)) {
          log("ID3D11ShaderReflection::GetDesc failed, hr=%#08x", hr);
          shader_refl->Release();
          blob->Release();
          return false; }
      }

      for (size_t p = 0; p < desc.InputParameters; ++p) {
        D3D11_SIGNATURE_PARAMETER_DESC param; {
          const HRESULT hr = shader_refl->GetInputParameterDesc(p, &param);
          if (FAILED(hr)) {
            log("ID3D11ShaderReflection::GetInputParamterDesc failed, hr=%#08x", hr);
            shader_refl->Release();
            blob->Release();
            return false; }
        }

        uint32_t component = 0;
        if (strcmp("POSITION", param.SemanticName) == 0)
          component = VertexDeclaration::POSITION;
        else if (strcmp("COLOR", param.SemanticName) == 0)
          component = VertexDeclaration::COLOR0 << param.SemanticIndex;
        else if (strcmp("TEXCOORD", param.SemanticName) == 0)
          component = VertexDeclaration::TEXCOORD0 << param.SemanticIndex;
        else if (strcmp("NORMAL", param.SemanticName) == 0)
          component = VertexDeclaration::NORMAL;
        else if (strcmp("TANGENT", param.SemanticName) == 0)
          component = VertexDeclaration::TANGENT;
        else if (strcmp("BINORMAL", param.SemanticName) == 0)
          component = VertexDeclaration::BITANGENT;
        else if (strcmp("BONEINDICES", param.SemanticName) == 0)
          component = VertexDeclaration::BONEINDICES;
        else if (strcmp("BONEWEIGHTS", param.SemanticName) == 0)
          component = VertexDeclaration::BONEWEIGHTS;
        else {
          log("Unknown vertex component (input semantic) '%s'!", param.SemanticName);
          shader_refl->Release();
          blob->Release();
          return false; }

        if (mrd.vertex_declaration.components() > component) {
          log("Vertex components (input semantics) are out of order.");
          log("  '%s' is out of order.", param.SemanticName);
          shader_refl->Release();
          blob->Release();
          return false; }

        mrd.vertex_declaration.components() |= component;
      }
    }

    shader_refl->Release();

    if (!File::write_out(cs.memory_resident_data(), (const void*)&mrd, offsetof(MemoryResidentData, byte_code))) {
      blob->Release();
      return false; }

    if (!File::write_out(cs.memory_resident_data(), (const void*)blob->GetBufferPointer(), blob->GetBufferSize())) {
      blob->Release();
      return false; }

    blob->Release();
    return true;
  }
} // butane
