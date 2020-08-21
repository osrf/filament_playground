/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/IndirectLight.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/Skybox.h>
#include <filament/TextureSampler.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>

#include <math/norm.h>

#include <utils/EntityManager.h>

#include <filameshio/MeshReader.h>

#include <image/KtxBundle.h>
#include <image/KtxUtility.h>

#include <filamentapp/Config.h>
#include <filamentapp/FilamentApp.h>
#include <filamentapp/IBL.h>

#include <stb_image.h>

#include "generated/resources/resources.h"
#include "generated/resources/drill.h"
#include "generated/resources/extinguisher.h"
#include "generated/resources/rescue_randy.h"
#include "generated/resources/pump.h"
#include "generated/resources/valve.h"

using namespace filament;
using namespace image;
using namespace filament::math;

struct GroundPlane {
    VertexBuffer* vb;
    IndexBuffer* ib;
    Material* mat;
    utils::Entity renderable;
};


struct App {
    Material* material;
    MaterialInstance* materialInstance;
    filamesh::MeshReader::Mesh mesh;
    mat4f transform;
    Texture* albedo;
    Texture* normal;
    Texture* roughness;
    Texture* metallic;
};

static GroundPlane createGroundPlane(Engine* engine);

static const char* IBL_FOLDER = "venetian_crossroads_2k";

static Texture* loadNormalMap(Engine* engine, const uint8_t* normals, size_t nbytes) {
    int w, h, n;
    unsigned char* data = stbi_load_from_memory(normals, nbytes, &w, &h, &n, 3);
    Texture* normalMap = Texture::Builder()
            .width(uint32_t(w))
            .height(uint32_t(h))
            .levels(0xff)
            .format(Texture::InternalFormat::RGB8)
            .build(*engine);
    Texture::PixelBufferDescriptor buffer(data, size_t(w * h * 3),
            Texture::Format::RGB, Texture::Type::UBYTE,
            (Texture::PixelBufferDescriptor::Callback) &stbi_image_free);
    normalMap->setImage(*engine, 0, std::move(buffer));
    normalMap->generateMipmaps(*engine);
    return normalMap;
}

int main(int argc, char** argv) {
    Config config;
    config.title = "suzanne";
    config.iblDirectory = FilamentApp::getRootAssetsPath() + IBL_FOLDER;

    App drillApp;
    App valveApp;
    App extinguisherApp;
    App rescueRandyApp;
    App pumpApp;
    auto setup = [config, &valveApp, &drillApp, &extinguisherApp, &rescueRandyApp, &pumpApp](Engine* engine, View* view, Scene* scene) {
        auto& tcm = engine->getTransformManager();
        auto& rcm = engine->getRenderableManager();
        auto& em = utils::EntityManager::get();

        // Create textures. The KTX bundles are freed by KtxUtility.
        auto valve_albedo = new image::KtxBundle(VALVE_VALVE_ALBEDO_S3TC_DATA, VALVE_VALVE_ALBEDO_S3TC_SIZE);
        auto valve_metallic = new image::KtxBundle(VALVE_VALVE_METALLIC_DATA, VALVE_VALVE_METALLIC_SIZE);
        auto valve_roughness = new image::KtxBundle(VALVE_VALVE_ROUGHNESS_DATA, VALVE_VALVE_ROUGHNESS_SIZE);
        valveApp.albedo = ktx::createTexture(engine, valve_albedo, true);
        valveApp.metallic = ktx::createTexture(engine, valve_metallic, false);
        valveApp.roughness = ktx::createTexture(engine, valve_roughness, false);
        valveApp.normal = loadNormalMap(engine, VALVE_VALVE_NORMAL_DATA, VALVE_VALVE_NORMAL_SIZE);

        auto drill_albedo = new image::KtxBundle(DRILL_DRILL_ALBEDO_S3TC_DATA, DRILL_DRILL_ALBEDO_S3TC_SIZE);
        auto drill_metallic = new image::KtxBundle(DRILL_DRILL_METALLIC_DATA, DRILL_DRILL_METALLIC_SIZE);
        auto drill_roughness = new image::KtxBundle(DRILL_DRILL_ROUGHNESS_DATA, DRILL_DRILL_ROUGHNESS_SIZE);
        drillApp.albedo = ktx::createTexture(engine, drill_albedo, true);
        drillApp.metallic = ktx::createTexture(engine, drill_metallic, false);
        drillApp.roughness = ktx::createTexture(engine, drill_roughness, false);
        drillApp.normal = loadNormalMap(engine, DRILL_DRILL_NORMAL_DATA, DRILL_DRILL_NORMAL_SIZE);

        auto extinguisher_albedo = new image::KtxBundle(EXTINGUISHER_EXTINGUISHER_ALBEDO_S3TC_DATA, EXTINGUISHER_EXTINGUISHER_ALBEDO_S3TC_SIZE);
        auto extinguisher_metallic = new image::KtxBundle(EXTINGUISHER_EXTINGUISHER_METALLIC_DATA, EXTINGUISHER_EXTINGUISHER_METALLIC_SIZE);
        auto extinguisher_roughness = new image::KtxBundle(EXTINGUISHER_EXTINGUISHER_ROUGHNESS_DATA, EXTINGUISHER_EXTINGUISHER_ROUGHNESS_SIZE);
        extinguisherApp.albedo = ktx::createTexture(engine, extinguisher_albedo, true);
        extinguisherApp.metallic = ktx::createTexture(engine, extinguisher_metallic, false);
        extinguisherApp.roughness = ktx::createTexture(engine, extinguisher_roughness, false);
        extinguisherApp.normal = loadNormalMap(engine, EXTINGUISHER_EXTINGUISHER_NORMAL_DATA, EXTINGUISHER_EXTINGUISHER_NORMAL_SIZE);

        auto rescue_randy_albedo = new image::KtxBundle(RESCUE_RANDY_RESCUE_RANDY_ALBEDO_S3TC_DATA, RESCUE_RANDY_RESCUE_RANDY_ALBEDO_S3TC_SIZE);
        auto rescue_randy_metallic = new image::KtxBundle(RESCUE_RANDY_RESCUE_RANDY_METALLIC_DATA, RESCUE_RANDY_RESCUE_RANDY_METALLIC_SIZE);
        auto rescue_randy_roughness = new image::KtxBundle(RESCUE_RANDY_RESCUE_RANDY_ROUGHNESS_DATA, RESCUE_RANDY_RESCUE_RANDY_ROUGHNESS_SIZE);
        rescueRandyApp.albedo = ktx::createTexture(engine, rescue_randy_albedo, true);
        rescueRandyApp.metallic = ktx::createTexture(engine, rescue_randy_metallic, false);
        rescueRandyApp.roughness = ktx::createTexture(engine, rescue_randy_roughness, false);
        rescueRandyApp.normal = loadNormalMap(engine, RESCUE_RANDY_RESCUE_RANDY_NORMAL_DATA, RESCUE_RANDY_RESCUE_RANDY_NORMAL_SIZE);

        auto pump_albedo = new image::KtxBundle(PUMP_PUMP_ALBEDO_S3TC_DATA, PUMP_PUMP_ALBEDO_S3TC_SIZE);
        auto pump_metallic = new image::KtxBundle(PUMP_PUMP_METALLIC_DATA, PUMP_PUMP_METALLIC_SIZE);
        auto pump_roughness = new image::KtxBundle(PUMP_PUMP_ROUGHNESS_DATA, PUMP_PUMP_ROUGHNESS_SIZE);
        pumpApp.albedo = ktx::createTexture(engine, pump_albedo, true);
        pumpApp.metallic = ktx::createTexture(engine, pump_metallic, false);
        pumpApp.roughness = ktx::createTexture(engine, pump_roughness, false);
        pumpApp.normal = loadNormalMap(engine, PUMP_PUMP_NORMAL_DATA, PUMP_PUMP_NORMAL_SIZE);

        TextureSampler sampler(TextureSampler::MinFilter::LINEAR_MIPMAP_LINEAR,
                TextureSampler::MagFilter::LINEAR,
                TextureSampler::WrapMode::REPEAT);

        // Instantiate material.
        valveApp.material = Material::Builder()
                .package(RESOURCES_TEXTUREDLIT_DATA, RESOURCES_TEXTUREDLIT_SIZE).build(*engine);
        valveApp.materialInstance = valveApp.material->createInstance();
        valveApp.materialInstance->setParameter("albedo", valveApp.albedo, sampler);
        valveApp.materialInstance->setParameter("metallic", valveApp.metallic, sampler);
        valveApp.materialInstance->setParameter("normal", valveApp.normal, sampler);
        valveApp.materialInstance->setParameter("roughness", valveApp.roughness, sampler);

        drillApp.material = Material::Builder()
                .package(RESOURCES_TEXTUREDLIT_DATA, RESOURCES_TEXTUREDLIT_SIZE).build(*engine);
        drillApp.materialInstance = valveApp.material->createInstance();
        drillApp.materialInstance->setParameter("albedo", drillApp.albedo, sampler);
        drillApp.materialInstance->setParameter("metallic", drillApp.metallic, sampler);
        drillApp.materialInstance->setParameter("normal", drillApp.normal, sampler);
        drillApp.materialInstance->setParameter("roughness", drillApp.roughness, sampler);

        extinguisherApp.material = Material::Builder()
                .package(RESOURCES_TEXTUREDLIT_DATA, RESOURCES_TEXTUREDLIT_SIZE).build(*engine);
        extinguisherApp.materialInstance = extinguisherApp.material->createInstance();
        extinguisherApp.materialInstance->setParameter("albedo", extinguisherApp.albedo, sampler);
        extinguisherApp.materialInstance->setParameter("metallic", extinguisherApp.metallic, sampler);
        extinguisherApp.materialInstance->setParameter("normal", extinguisherApp.normal, sampler);
        extinguisherApp.materialInstance->setParameter("roughness", extinguisherApp.roughness, sampler);

        rescueRandyApp.material = Material::Builder()
                .package(RESOURCES_TEXTUREDLIT_DATA, RESOURCES_TEXTUREDLIT_SIZE).build(*engine);
        rescueRandyApp.materialInstance = rescueRandyApp.material->createInstance();
        rescueRandyApp.materialInstance->setParameter("albedo", rescueRandyApp.albedo, sampler);
        rescueRandyApp.materialInstance->setParameter("metallic", rescueRandyApp.metallic, sampler);
        rescueRandyApp.materialInstance->setParameter("normal", rescueRandyApp.normal, sampler);
        rescueRandyApp.materialInstance->setParameter("roughness", rescueRandyApp.roughness, sampler);

        pumpApp.material = Material::Builder()
                .package(RESOURCES_TEXTUREDLIT_DATA, RESOURCES_TEXTUREDLIT_SIZE).build(*engine);
        pumpApp.materialInstance = pumpApp.material->createInstance();
        pumpApp.materialInstance->setParameter("albedo", pumpApp.albedo, sampler);
        pumpApp.materialInstance->setParameter("metallic", pumpApp.metallic, sampler);
        pumpApp.materialInstance->setParameter("normal", pumpApp.normal, sampler);
        pumpApp.materialInstance->setParameter("roughness", pumpApp.roughness, sampler);

        auto ibl = FilamentApp::get().getIBL()->getIndirectLight();
        ibl->setIntensity(100000);
        ibl->setRotation(mat3f::rotation(0.5f, float3{ 0, 1, 0 }));

        // Add geometry into the scene.
        valveApp.mesh = filamesh::MeshReader::loadMeshFromBuffer(engine, VALVE_VALVE_DATA, nullptr,
                nullptr, valveApp.materialInstance);
        auto valve_ti = tcm.getInstance(valveApp.mesh.renderable);
        valveApp.transform = mat4f{ mat3f(1), float3(3, 0, -4) } * tcm.getWorldTransform(valve_ti);
        rcm.setCastShadows(rcm.getInstance(valveApp.mesh.renderable), true);
        scene->addEntity(valveApp.mesh.renderable);
        tcm.setTransform(valve_ti, valveApp.transform);

        drillApp.mesh = filamesh::MeshReader::loadMeshFromBuffer(engine, DRILL_DRILL_DATA, nullptr,
                nullptr, drillApp.materialInstance);
        auto drill_ti = tcm.getInstance(drillApp.mesh.renderable);
        drillApp.transform = mat4f{ mat3f(1), float3(0, 0, -4) } * tcm.getWorldTransform(drill_ti);
        rcm.setCastShadows(rcm.getInstance(drillApp.mesh.renderable), true);
        scene->addEntity(drillApp.mesh.renderable);
        tcm.setTransform(drill_ti, drillApp.transform);

        extinguisherApp.mesh = filamesh::MeshReader::loadMeshFromBuffer(engine, EXTINGUISHER_EXTINGUISHER_DATA, nullptr,
                nullptr, extinguisherApp.materialInstance);
        auto extinguisher_ti = tcm.getInstance(extinguisherApp.mesh.renderable);
        extinguisherApp.transform = mat4f{ mat3f(1), float3(1, 0, -4) } * tcm.getWorldTransform(extinguisher_ti);
        rcm.setCastShadows(rcm.getInstance(extinguisherApp.mesh.renderable), true);
        scene->addEntity(extinguisherApp.mesh.renderable);
        tcm.setTransform(extinguisher_ti, extinguisherApp.transform);

        rescueRandyApp.mesh = filamesh::MeshReader::loadMeshFromBuffer(engine, RESCUE_RANDY_RESCUE_RANDY_DATA, nullptr,
                nullptr, rescueRandyApp.materialInstance);
        auto rescue_randy_ti = tcm.getInstance(rescueRandyApp.mesh.renderable);
        rescueRandyApp.transform = mat4f{ mat3f(1), float3(2, 0, -4) } * tcm.getWorldTransform(rescue_randy_ti);
        rcm.setCastShadows(rcm.getInstance(rescueRandyApp.mesh.renderable), true);
        scene->addEntity(rescueRandyApp.mesh.renderable);
        tcm.setTransform(rescue_randy_ti, rescueRandyApp.transform);

        pumpApp.mesh = filamesh::MeshReader::loadMeshFromBuffer(engine, PUMP_PUMP_DATA, nullptr,
                nullptr, pumpApp.materialInstance);
        auto pump_ti = tcm.getInstance(pumpApp.mesh.renderable);
        // pumpApp.transform = mat4f{ mat3f(1), float3(4, 0, -4) } * tcm.getWorldTransform(pump_ti);
        pumpApp.transform = mat4f::translation(float3{ 4, 0, -4, }) * mat4f::rotation(1.57, float3{ 0, 1, 0 });
        rcm.setCastShadows(rcm.getInstance(pumpApp.mesh.renderable), true);
        scene->addEntity(pumpApp.mesh.renderable);
        tcm.setTransform(pump_ti, pumpApp.transform);

        utils::Entity light = utils::EntityManager::get().create();
        // LightManager::Builder(LightManager::Type::DIRECTIONAL)
        LightManager::Builder(LightManager::Type::SUN)
                .color(Color::toLinear<ACCURATE>({0.98f, 0.92f, 0.89f}))
                .intensity(110000)
                .direction({0.2, -1, -0.8})
                .sunAngularRadius(1.9f)
                .castShadows(true)
                .build(*engine, light);
        scene->addEntity(light);

        // utils::Entity sp1 = utils::EntityManager::get().create();
        // LightManager::Builder(LightManager::Type::SPOT)
        //         .color(Color::toLinear<ACCURATE>(sRGBColor(0.9f, 0.9f, 0.9f)))
        //         .intensity(110000.0f)
        //         .position({ -3.0f, 8.0f, -1.5f })
        //         .direction({ 0.0f, -1.0f, 0.0f })
        //         .spotLightCone(static_cast<float>(M_PI / 8), static_cast<float>(1.5707))
        //         .falloff(10.0f)
        //         .castShadows(false)
        //         .build(*engine, sp1);
        // scene->addEntity(sp1);

        // utils::Entity sp2 = utils::EntityManager::get().create();
        // LightManager::Builder(LightManager::Type::SPOT)
        //         .color(Color::toLinear<ACCURATE>(sRGBColor(0.98f, 0.12f, 0.19f)))
        //         // .intensity(12000.0f, LightManager::EFFICIENCY_LED)
        //         .intensity(110000.0f)
        //         .position({ 7.0f, 8.0f, -1.5f })
        //         .direction({ 0.0f, -1.0f, 0.0f })
        //         .spotLightCone(static_cast<float>(M_PI / 8), static_cast<float>(1.5707))
        //         .falloff(1.0f)
        //         .castShadows(true)
        //         .build(*engine, sp2);
        // scene->addEntity(sp2);



        Skybox *skybox = Skybox::Builder().color({0.5f,0.75f,1.0f,1.0f}).build(*engine);
        scene->setSkybox(skybox);

        GroundPlane plane = createGroundPlane(engine);
        scene->addEntity(plane.renderable);
    };

    auto cleanup = [&valveApp, &drillApp, &extinguisherApp, &rescueRandyApp, &pumpApp](Engine* engine, View*, Scene*) {
        engine->destroy(valveApp.materialInstance);
        engine->destroy(valveApp.mesh.renderable);
        engine->destroy(valveApp.material);
        engine->destroy(valveApp.albedo);
        engine->destroy(valveApp.normal);
        engine->destroy(valveApp.roughness);
        engine->destroy(valveApp.metallic);

        engine->destroy(drillApp.materialInstance);
        engine->destroy(drillApp.mesh.renderable);
        engine->destroy(drillApp.material);
        engine->destroy(drillApp.albedo);
        engine->destroy(drillApp.normal);
        engine->destroy(drillApp.roughness);
        engine->destroy(drillApp.metallic);

        engine->destroy(extinguisherApp.materialInstance);
        engine->destroy(extinguisherApp.mesh.renderable);
        engine->destroy(extinguisherApp.material);
        engine->destroy(extinguisherApp.albedo);
        engine->destroy(extinguisherApp.normal);
        engine->destroy(extinguisherApp.roughness);
        engine->destroy(extinguisherApp.metallic);

        engine->destroy(rescueRandyApp.materialInstance);
        engine->destroy(rescueRandyApp.mesh.renderable);
        engine->destroy(rescueRandyApp.material);
        engine->destroy(rescueRandyApp.albedo);
        engine->destroy(rescueRandyApp.normal);
        engine->destroy(rescueRandyApp.roughness);
        engine->destroy(rescueRandyApp.metallic);

        engine->destroy(pumpApp.materialInstance);
        engine->destroy(pumpApp.mesh.renderable);
        engine->destroy(pumpApp.material);
        engine->destroy(pumpApp.albedo);
        engine->destroy(pumpApp.normal);
        engine->destroy(pumpApp.roughness);
        engine->destroy(pumpApp.metallic);
    };


    // config.backend = Engine::Backend::VULKAN;
    FilamentApp::get().run(config, setup, cleanup);

    return 0;
}

static GroundPlane createGroundPlane(Engine* engine) {
    Material* shadowMaterial = Material::Builder()
        .package(RESOURCES_GROUNDSHADOW_DATA, RESOURCES_GROUNDSHADOW_SIZE)
        .build(*engine);
    shadowMaterial->setDefaultParameter("strength", 0.7f);

    const static uint32_t indices[] {
        0, 1, 2, 2, 3, 0
    };
    const static float3 vertices[] {
        { -10, 0, -10 },
        { -10, 0,  10 },
        {  10, 0,  10 },
        {  10, 0, -10 },
    };
    short4 tbn = packSnorm16(normalize(positive(mat3f{
        float3{1.0f, 0.0f, 0.0f}, float3{0.0f, 0.0f, 1.0f}, float3{0.0f, 1.0f, 0.0f}
    }.toQuaternion())).xyzw);
    const static short4 normals[] { tbn, tbn, tbn, tbn };
    VertexBuffer* vertexBuffer = VertexBuffer::Builder()
        .vertexCount(4)
        .bufferCount(2)
        .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT3)
        .attribute(VertexAttribute::TANGENTS, 1, VertexBuffer::AttributeType::SHORT4)
        .normalized(VertexAttribute::TANGENTS)
        .build(*engine);
    vertexBuffer->setBufferAt(*engine, 0, VertexBuffer::BufferDescriptor(
            vertices, vertexBuffer->getVertexCount() * sizeof(vertices[0])));
    vertexBuffer->setBufferAt(*engine, 1, VertexBuffer::BufferDescriptor(
            normals, vertexBuffer->getVertexCount() * sizeof(normals[0])));
    IndexBuffer* indexBuffer = IndexBuffer::Builder().indexCount(6).build(*engine);
    indexBuffer->setBuffer(*engine, IndexBuffer::BufferDescriptor(
            indices, indexBuffer->getIndexCount() * sizeof(uint32_t)));

    auto& em = utils::EntityManager::get();
    utils::Entity renderable = em.create();
    RenderableManager::Builder(1)
        .boundingBox({{ 0, 0, 0 }, { 10, 1e-4f, 10 }})
        .material(0, shadowMaterial->getDefaultInstance())
        .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, vertexBuffer, indexBuffer, 0, 6)
        .culling(false)
        .receiveShadows(true)
        .castShadows(false)
        .build(*engine, renderable);

    auto& tcm = engine->getTransformManager();
    tcm.setTransform(tcm.getInstance(renderable), mat4f::translation(float3{ 3, 0, -4 }));
    return {
        .vb = vertexBuffer,
        .ib = indexBuffer,
        .mat = shadowMaterial,
        .renderable = renderable,
    };
}
