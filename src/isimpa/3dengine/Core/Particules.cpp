/* ----------------------------------------------------------------------
* I-SIMPA (http://i-simpa.ifsttar.fr). This file is part of I-SIMPA.
*
* I-SIMPA is a GUI for 3D numerical sound propagation modelling dedicated
* to scientific acoustic simulations.
* Copyright (C) 2007-2014 - IFSTTAR - Judicael Picaut, Nicolas Fortin
*
* I-SIMPA is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
* 
* I-SIMPA is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA or 
* see <http://ww.gnu.org/licenses/>
*
* For more information, please consult: <http://i-simpa.ifsttar.fr> or 
* send an email to i-simpa@ifsttar.fr
*
* To contact Ifsttar, write to Ifsttar, 14-20 Boulevard Newton
* Cite Descartes, Champs sur Marne F-77447 Marne la Vallee Cedex 2 FRANCE
* or write to scientific.computing@ifsttar.fr
* ----------------------------------------------------------------------*/

#include "GL/opengl_inc.h"
#include "Particules.h"
#include <wx/log.h>
#include <wx/progdlg.h>
#include "manager/stringTools.h"
#include "data_manager/appconfig.h"
#include <fstream>
#include <wx/filename.h>
#include <wx/utils.h>
#include "input_output/particles/part_io.hpp"
#include <limits>
#include <math.h>
#include "last_cpp_include.hpp"

#define __PARTICLE_COLOR__ 0
#define CHAR_TAB 9
#define CHAR_RETURN_WIN 13
#define CHAR_RETURN_UNIX 10
#define CHAR_FINCHAINE 0
#define TIMESTEP_LIMITATION 1000000

ParticulesControler::ParticulesControler()
: Animator(),tabInfoParticles(NULL)
{
	this->SetRendererName("particules");
	nbStep=0;
	nbParticles=0;
	legendRenderer=NULL;
	min_energy = (std::numeric_limits<float>::max)();
	max_energy = (std::numeric_limits<float>::min)();
}


wxString ParticulesControler::GetRendererLabel()
{
	return _("Particles");
}
ParticulesControler::~ParticulesControler()
{
	delete[] tabInfoParticles;
}

int ParticulesControler::GetNbTimeStep()
{
	return nbStep;
}

void ParticulesControler::SetTimeStep(const int& _timeStp)
{
	if(this->p_legends.currentTimeStep)
	{
		if(this->nbStep>0)
		{
			this->p_legends.currentTimeStep->InitText(Convertor::ToString(timeStep*(_timeStp+1))+" s");
		}
	}
}
int ParticulesControler::GetSizeTabParticle()
{
	return this->nbParticles;
}
void ParticulesControler::Init(const bool& resetLoadingTime)
{
	delete[] tabInfoParticles;
	this->tabInfoParticles = NULL;
	this->nbParticles=0;
	nbStep=0;
	this->timeStep=0.f;
	if(legendRenderer)
	{
		if(this->p_legends.currentTimeStep)
		{
			legendRenderer->Delete(this->p_legends.currentTimeStep);
			this->p_legends.currentTimeStep=NULL;
		}
		if(this->p_legends.currentFile)
		{
			legendRenderer->Delete(this->p_legends.currentFile);
			this->p_legends.currentFile=NULL;
		}
	}
	Animator::Init(resetLoadingTime);
}

void ParticulesControler::InitLegend(legendRendering::ForeGroundGlBitmap& _legendRenderer)
{
	legendRenderer=&_legendRenderer;
}


void ParticulesControler::SetTextForegroundColor(const ivec4& foregroundColor)
{
	this->text_foreground_color.Set(foregroundColor.red,foregroundColor.green,foregroundColor.blue,foregroundColor.alpha);
}
void ParticulesControler::SetTextBackgroundColor(const ivec4& backgroundColor)
{
	this->text_background_color.Set(backgroundColor.red,backgroundColor.green,backgroundColor.blue,backgroundColor.alpha);
}
void ParticulesControler::SetParticleColor(const ivec3& _particleColor)
{
	particleColor.set(_particleColor.a/255.f,_particleColor.b/255.f,_particleColor.c/255.f,1.f);
}
void ParticulesControler::RefreshAnimationRendering()
{
	loadingTime=wxDateTime::Now();
}
void ParticulesControler::SetTextFont(const wxFont& textFont)
{
	this->legendFont=textFont;
}

void ParticulesControler::LoadPBin(wxString fileName, bool doCoordsTransformation, vec4 UnitizeVal)
{

	Init(false);
	using namespace std;
	using namespace particleio;

	ParticuleIO driver;

	if(driver.OpenForRead(fileName.ToStdString())) {
		unsigned long nbstepmax;
		driver.GetHeaderData(this->timeStep, this->nbParticles, nbstepmax);
		this->nbStep = nbstepmax;

		wxFileName filePath = fileName;
		filePath.MakeRelativeTo(ApplicationConfiguration::GLOBAL_VAR.cacheFolderPath + ApplicationConfiguration::CONST_REPORT_FOLDER_PATH);
		if (filePath.GetDirCount()>2)
		{
			filePath.RemoveLastDir();
			filePath.RemoveLastDir();
		}
		filePath.ClearExt();
		filePath.SetName("");
		ShortcutPath = filePath.GetFullPath();


		// Precomputre required memory
		wxMemorySize freemem = wxGetFreeMemory();
		wxMemorySize memrequire = (wxMemorySize)(sizeof(double) * 4 * nbParticles * nbStep);

		if (freemem<memrequire && freemem != -1)
		{
			wxLogError(_("Insufficent memory available, %u memory required and %u memory available"), (unsigned int)(memrequire / 1.e6).ToLong(), (unsigned int)(freemem / 1.e6).ToLong());
			Init();
			driver.Close();
			return;
		}		
		wxLogDebug(_("Particles file contains %i particles and %i time step"), this->nbParticles, this->nbStep);
		if (this->nbParticles>0)
		{
			wxProgressDialog progDialog(_("Loading particles file"), _("Loading particles file"), 100, NULL, wxPD_CAN_ABORT | wxPD_REMAINING_TIME | wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE);
			progDialog.Update(0);
			tabInfoParticles = new t_ParticuleInfo[this->nbParticles];
			int percProgression = 0;
			int ansPercProgression = 0;
			for (unsigned long pIndex = 0; pIndex<this->nbParticles; pIndex++)
			{
				percProgression = ((float)pIndex / this->nbParticles) * 100;
				if (ansPercProgression != percProgression && percProgression<100)
				{
					ansPercProgression = percProgression;
					if (!progDialog.Update(percProgression))
					{
						//Annulation du chargement du fichier par l'utilisateur
						Init();
						wxLogMessage(_("Cancel loading particles files"));
						driver.Close();
						return;
					}
				}
				unsigned long firstTimeStep, nbTimeStep;
				driver.NextParticle(firstTimeStep, nbTimeStep);
				tabInfoParticles[pIndex].nbStep = nbTimeStep;
				tabInfoParticles[pIndex].firstStep = firstTimeStep;
				if (nbTimeStep>0 && nbTimeStep < TIMESTEP_LIMITATION)
				{
					tabInfoParticles[pIndex].tabSteps = new t_Particule[nbTimeStep];

					for (unsigned long pTimeIndex = 0; pTimeIndex < nbTimeStep; pTimeIndex++)
					{
						float energy;
						vec3 position;
						driver.NextTimeStep(position.x, position.y, position.z, energy);
						if (doCoordsTransformation)
							position = coordsOperation::CommonCoordsToGlCoords(UnitizeVal, position);

						tabInfoParticles[pIndex].tabSteps[pTimeIndex].pos[0] = position[0];
						tabInfoParticles[pIndex].tabSteps[pTimeIndex].pos[1] = position[1];
						tabInfoParticles[pIndex].tabSteps[pTimeIndex].pos[2] = position[2];
						tabInfoParticles[pIndex].tabSteps[pTimeIndex].energie = energy;

						if (energy > max_energy)
							max_energy = energy;
						if (energy < min_energy)
							min_energy = energy;
					}
				}
			}
			EnableRendering();
		}
	}

	//Chargement et mis à jour des légendes
	RedrawLegend();
	loadingTime=wxDateTime::Now();
}

bool  ParticulesControler::LoadParticleFile(const char *mfilename, vec4 UnitizeVal, Element* elConf, Element::IDEVENT loadingMethod)
{
	renderMethod=loadingMethod;
	this->Init(false);
    if (!wxFileExists(mfilename))
    {
		wxLogError(_("Particles file does not exist!\n %s"),mfilename);
        return false;
    }

	try
	{
		LoadPBin(mfilename,true,UnitizeVal);
	}
	catch( ... ) {
		wxLogError(_("Unknown error when reading particles file"));
		return false;
	}
	return true;
}

void ParticulesControler::RedrawLegend()
{
	if(!this->tabInfoParticles || !this->IsRenderingEnable())
		return;
	using namespace legendRendering;
	if(legendRenderer)
	{

		legendText* fileLegend;


		if(!this->p_legends.currentFile)
		{
			fileLegend=new legendText();
		}else{
			fileLegend=this->p_legends.currentFile;
		}
		fileLegend->SetFont( this->legendFont );
		fileLegend->SetTextForeground(this->text_foreground_color);
		fileLegend->SetTextBackground(this->text_background_color);
		fileLegend->InitText(_("Particles:")+ShortcutPath,35);


		if(!this->p_legends.currentFile)
		{
			legendCfg currentObjConf=fileLegend->GetCfg();
			currentObjConf.alignementV=ALIGNEMENT_V_TOP;
			currentObjConf.alignementH=ALIGNEMENT_H_LEFT;
			fileLegend->SetCfg(currentObjConf);
			this->p_legends.currentFile=fileLegend;
			legendRenderer->Push(this->p_legends.currentFile);
		}
	}
	if(legendRenderer)
	{

		legendText* fileLegend;


		if(!this->p_legends.currentTimeStep)
		{
			fileLegend=new legendText();
		}else{
			fileLegend=this->p_legends.currentTimeStep;
		}
		fileLegend->SetFont( this->legendFont );
		fileLegend->SetTextForeground(this->text_foreground_color);
		fileLegend->SetTextBackground(this->text_background_color);
		fileLegend->InitText("0,000 s",8);


		if(!this->p_legends.currentTimeStep)
		{
			legendCfg currentObjConf=fileLegend->GetCfg();
			currentObjConf.alignementV=ALIGNEMENT_V_TOP;
			currentObjConf.alignementH=ALIGNEMENT_H_LEFT;
			fileLegend->SetCfg(currentObjConf);
			this->p_legends.currentTimeStep=fileLegend;
			legendRenderer->Push(this->p_legends.currentTimeStep);
		}
	}
}

void ParticulesControler::Render(const int& timeStp)
{
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);	//glEnable(GL_BLEND);
	glPointSize(2);
	//Antialiasing sur les points
	glDisable (GL_POINT_SMOOTH);
	glDisable(GL_TEXTURE_2D);
	//Gestion de la transparence
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(renderMethod==Element::IDEVENT_LOAD_PARTICLE_SIMULATION)
	{
		glBegin(GL_POINTS);
		glColor4f(particleColor[0], particleColor[1], particleColor[2], 1.);
		
		#if __PARTICLE_COLOR__
			// linear to decibel scale
			max_energy = 10 * (log(max_energy) / log(10));
			min_energy = 10 * (log(min_energy) / log(10));

			auto echelon1 = max_energy - 5;
			auto echelon2 = max_energy - 10;
			auto echelon3 = max_energy - 15;
			auto echelon4 = max_energy - 20;
			auto echelon5 = max_energy - 25;
		#endif

		for(unsigned int i=0;i<nbParticles;i++)
		{
			if(tabInfoParticles[i].nbStep>timeStp-tabInfoParticles[i].firstStep && tabInfoParticles[i].tabSteps)
			{
				#if __PARTICLE_COLOR__
					// color in function of energy
					float particle_energy = 10 * (log(tabInfoParticles[i].tabSteps[timeStp - tabInfoParticles[i].firstStep].energie) / log(10));
					if (particle_energy > echelon1)
						glColor4f(1.0, 0.0, 0.0, 1.);
					else if (particle_energy > echelon2)
						glColor4f(1.0, 0.5, 0.0, 1.);
					else if (particle_energy > echelon3)
						glColor4f(1.0, 1.0, 0.0, 1.);
					else if (particle_energy > echelon4)
						glColor4f(0.5, 1.0, 0.0, 1.);
					else if (particle_energy > echelon5)
						glColor4f(0.0, 1.0, 1.0, 1.);
					else
						glColor4f(0.0, 0.5, 1.0, 1.);
				#endif
				
				glVertex3fv(tabInfoParticles[i].tabSteps[timeStp-tabInfoParticles[i].firstStep].pos);
			}
		}
		glEnd();
	}else if(renderMethod==Element::IDEVENT_LOAD_PARTICLE_SIMULATION_PATH){
		//On fait le tracé entre deux positions pas de temps -1 vers pas de temps demandé
		glBegin(GL_LINES);
		glColor4f(particleColor[0],particleColor[1],particleColor[2],1.);
		for(unsigned int i=0;i<nbParticles;i++)
		{
			if(tabInfoParticles[i].nbStep>timeStp-tabInfoParticles[i].firstStep && tabInfoParticles[i].tabSteps && int(timeStp)-int(tabInfoParticles[i].firstStep)>0)
			{
				glVertex3fv(tabInfoParticles[i].tabSteps[timeStp-tabInfoParticles[i].firstStep-1].pos);
				glVertex3fv(tabInfoParticles[i].tabSteps[timeStp-tabInfoParticles[i].firstStep].pos);
			}
		}
		glEnd();
	}
}
