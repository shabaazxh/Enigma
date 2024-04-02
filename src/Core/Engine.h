#pragma once

#include <Volk/volk.h>
#include "../Core/Error.h"
#include "../Graphics/VulkanContext.h"
#include "../Graphics/VulkanObjects.h"
#include <GLFW/glfw3.h>
#include "../Graphics/Light.h"
#include <fstream>
#include "../Core/Error.h"
#include <string>

namespace Enigma
{
	class Settings
	{
		protected:
			Settings() = default;	
		public:
			Settings(const Settings&) = delete;
			static Settings& Get()
			{
				static Settings instance;
				return instance;
			}

			static void MIP_VISUAL_ON()  { Get().SetMipVisual(true); };
			static void MIP_VISUAL_OFF() { Get().SetMipVisual(false); }

			static void OVERDRAW_VISUAL_ON()  { Get().SetOverdrawVisual(true); }
			static void OVERDRAW_VISUAL_OFF() { Get().SetOverdrawVisual(false); }

			static void OVERSHADIING_VISUAL_ON() { Get().SetOvershadingVisual(true); }
			static void OVERSHADING_VISUAL_OFF() { Get().SetOverdrawVisual(false); }

			static void MESH_DENSITY_VISUAL_ON() { Get().SetMeshDensityVisual(true); }
			static void MESH_DENSITY_VISUAL_OFF() { Get().SetMeshDensityVisual(false); }

			static bool MIP_VISUAL() { return Get().GetMipVisualImp(); }
			static bool OVERDRAW_VISUAL() { return Get().GetOverdrawVisualImp(); }
			static bool OVERSHADE_VISUAL() { return Get().GetOvershadingVisualImpl(); }
			static bool MESH_DENSITY_VISUAL() { return Get().GetMeshDensityVisualImp(); }

		private:
			void SetMipVisual(bool s) {

				if (s) {
					m_enableMipVisual = true;
					m_enableOverdrawVisual = false;
					m_enableOvershadingVisual = false;
					m_enableMeshDensityVisual = false;
				}
				else {
					m_enableMipVisual = false;
				}
			}

			void SetOverdrawVisual(bool s)
			{
				if (s) {
					m_enableMipVisual = false;
					m_enableOverdrawVisual = true;
					m_enableOvershadingVisual = false;
					m_enableMeshDensityVisual = false;
				}
				else {
					m_enableOverdrawVisual = false;
				}
			}

			void SetOvershadingVisual(bool s)
			{
				if (s) {
					m_enableMipVisual = false;
					m_enableOverdrawVisual = false;
					m_enableOvershadingVisual = true;
					m_enableMeshDensityVisual = false;
				}
				else {
					m_enableOvershadingVisual = false;
				}
			}

			void SetMeshDensityVisual(bool s)
			{
				if (s) {
					m_enableMipVisual = false;
					m_enableOverdrawVisual = false;
					m_enableOvershadingVisual = false;
					m_enableMeshDensityVisual = true;
				}
				else {
					m_enableMeshDensityVisual = false;
				}
			}

			bool GetMipVisualImp() const { return m_enableMipVisual; }
			bool GetOverdrawVisualImp() const { return m_enableOverdrawVisual; }
			bool GetOvershadingVisualImpl() const { return m_enableOvershadingVisual; }
			bool GetMeshDensityVisualImp() const { return m_enableMeshDensityVisual; }

			bool m_enableMipVisual = false;
			bool m_enableOverdrawVisual = false;
			bool m_enableOvershadingVisual = false;
			bool m_enableMeshDensityVisual = false;
	};


}