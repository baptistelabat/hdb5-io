//
// Created by lletourn on 26/02/20.
//

#include <Eigen/Dense>

#include "HydrodynamicDataBase.h"

#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <highfive/H5File.hpp>
#include <highfive/H5Easy.hpp>

namespace HDB5_io {


  void HydrodynamicDataBase::Import_HDF5(const std::string &HDF5_file) {

  using namespace HighFive;

    File file(HDF5_file, File::ReadOnly);

    try {
      file.getDataSet("Version").read(m_version);
    } catch (Exception &err) {
      std::cerr << err.what() << std::endl;
      m_version = 1.0;
    }

    Import_HDF5_v3(HDF5_file);

  }

  void HydrodynamicDataBase::Import_HDF5_v3(const std::string &HDF5_file) {

    using namespace HighFive;

    File file(HDF5_file, File::ReadOnly);

    file.getDataSet("CreationDate").read(m_creationDate);
    file.getDataSet("Solver").read(m_solver);
    file.getDataSet("NbBody").read(m_nbody);
    file.getDataSet("NormalizationLength").read(m_normalizationLength);
    file.getDataSet("GravityAcc").read(m_gravityAcceleration);
    file.getDataSet("WaterDensity").read(m_waterDensity);
    file.getDataSet("WaterDepth").read(m_waterDepth);

    double min, max;
    unsigned int nb;
    auto disc = file.getGroup("Discretizations").getGroup("Frequency");
    disc.getDataSet("MinFrequency").read(min);
    disc.getDataSet("MaxFrequency").read(max);
    disc.getDataSet("NbFrequencies").read(nb);
    m_frequencyDiscretization = {min, max, nb};

    disc = file.getGroup("Discretizations").getGroup("Time");
    disc.getDataSet("TimeStep").read(min);
    disc.getDataSet("FinalTime").read(max);
    disc.getDataSet("NbTimeSample").read(nb);
//    assert(abs(max/double(nb) - min) < 1E-5);
    m_timeDiscretization = {0., max, nb};

    disc = file.getGroup("Discretizations").getGroup("WaveDirections");
    disc.getDataSet("MinAngle").read(min);
    disc.getDataSet("MaxAngle").read(max);
    disc.getDataSet("NbWaveDirections").read(nb);
    m_waveDirectionDiscretization = {min, max, nb};


    for (int i = 0; i < m_nbody; i++) {
      std::string name;
      unsigned int id;

      auto hdb_body = file.getGroup("Bodies").getGroup("Body_" + std::to_string(i));

      hdb_body.getDataSet("BodyName").read(name);
      hdb_body.getDataSet("ID").read(id);

      auto body = NewBody(id, name);

//      std::vector<double> position;
//      hdb_body.getDataSet("BodyPosition").read(position);
//      body->SetPosition(Eigen::Vector3d(position.data()));

//      Eigen::Vector3d position;
//      hdb_body.getDataSet("BodyPosition").read(position);
      mathutils::Vector3d<double> position;
      position = H5Easy::load<Eigen::Vector3d>(file, "Bodies/Body_" + std::to_string(i) + "/BodyPosition");
      body->SetPosition(position);

//      std::vector<int> mask;
//      hdb_body.getGroup("Mask").getDataSet("ForceMask").read(mask);
//      body->SetForceMask(Eigen::Map<Eigen::VectorXi, Eigen::Unaligned>(mask.data(), mask.size()));
//      hdb_body.getGroup("Mask").getDataSet("MotionMask").read(mask);
//      body->SetMotionMask(Eigen::Map<Eigen::VectorXi, Eigen::Unaligned>(mask.data(), mask.size()));

      mathutils::Vector6d<int> mask;
      mask = H5Easy::load<Eigen::Matrix<int, 6, 1>>(file, "Bodies/Body_" + std::to_string(i) + "/Mask/ForceMask");
      body->SetForceMask(mask);
      mask = H5Easy::load<Eigen::Matrix<int, 6, 1>>(file, "Bodies/Body_" + std::to_string(i) + "/Mask/MotionMask");
      body->SetMotionMask(mask);

      body->AllocateAll(m_frequencyDiscretization.GetNbSample(), m_waveDirectionDiscretization.GetNbSample());
    }


    for (auto &body : m_bodies) {
      ReadExcitation(file, "Bodies/Body_" + std::to_string(body->GetID()) , body.get());
    }

  }


  HydrodynamicDataBase::HydrodynamicDataBase() {

  }

//  void HydrodynamicDataBase::ReadExcitation(HighFive::Group* group, Body* body) {
//
//    auto forceMask = body->GetForceMask();
//
//    for (unsigned int iwaveDir = 0; iwaveDir < m_waveDirectionDiscretization.GetNbSample(); ++iwaveDir) {
//
//      auto angle_group = group->getGroup("Excitation/Diffraction/Angle_" + std::to_string(iwaveDir));
//
//      double angle;
//      angle_group.getDataSet("Angle").read(angle);
//      assert(abs(m_waveDirectionDiscretization.GetVector()[iwaveDir] - angle) < 1E-5);
//
//      std::vector<std::vector<double>> coeffs;
//      angle_group.getDataSet("ImagCoeffs").read(coeffs);
//      std::cout<<coeffs[0][1]<<std::endl;
//
//      for (unsigned int idof = 0; idof < 6; idof++){}
//
//
//    }
//
//  }

  void HydrodynamicDataBase::ReadExcitation(const HighFive::File &HDF5_file, const std::string &path, Body* body) {

    auto forceMask = body->GetForceMask();

    auto diffractionPath = path + "/Excitation/Diffraction";
    auto froudeKrylovPath = path + "/Excitation/FroudeKrylov/";

    for (unsigned int iwaveDir = 0; iwaveDir < m_waveDirectionDiscretization.GetNbSample(); ++iwaveDir) {

      auto diffractionWaveDirPath = diffractionPath + "/Angle_" + std::to_string(iwaveDir);

      auto angle = H5Easy::load<double>(HDF5_file, diffractionWaveDirPath + "/Angle");
      assert(abs(m_waveDirectionDiscretization.GetVector()[iwaveDir] - angle) < 1E-5);

      auto realCoeffs = H5Easy::load<Eigen::MatrixXd>(HDF5_file, diffractionWaveDirPath + "/RealCoeffs");
      auto imagCoeffs = H5Easy::load<Eigen::MatrixXd>(HDF5_file, diffractionWaveDirPath + "/ImagCoeffs");
      auto Dcoeffs = realCoeffs  + MU_JJ * imagCoeffs;

      Eigen::MatrixXcd DiffractionCoeffs;
      if (imagCoeffs.rows() != forceMask.GetNbDOF()) {
        // Condense the matrix by removing the lines corresponding to the masked DOFs
        DiffractionCoeffs = Eigen::VectorXi::Map(forceMask.GetListDOF().data(), forceMask.GetNbDOF()).replicate(1,Dcoeffs.cols()).unaryExpr(Dcoeffs);
      } else{
        DiffractionCoeffs = Dcoeffs;
      }
      body->SetDiffraction(iwaveDir, DiffractionCoeffs);

      auto froudeKrylovWaveDirPath = froudeKrylovPath + "/Angle_" + std::to_string(iwaveDir);
      realCoeffs = H5Easy::load<Eigen::MatrixXd>(HDF5_file, froudeKrylovWaveDirPath + "/RealCoeffs");
      imagCoeffs = H5Easy::load<Eigen::MatrixXd>(HDF5_file, froudeKrylovWaveDirPath + "/ImagCoeffs");
      auto FKcoeffs = realCoeffs  + MU_JJ * imagCoeffs;

      Eigen::MatrixXcd froudeKrylovCoeffs;
      if (imagCoeffs.rows() != forceMask.GetNbDOF()) {
        // Condense the matrix by removing the lines corresponding to the masked DOFs
        froudeKrylovCoeffs = Eigen::VectorXi::Map(forceMask.GetListDOF().data(), forceMask.GetNbDOF()).replicate(1,FKcoeffs.cols()).unaryExpr(FKcoeffs);
      } else{
        froudeKrylovCoeffs = FKcoeffs;
      }
      body->SetFroudeKrylov(iwaveDir, froudeKrylovCoeffs);

      body->SetExcitation(iwaveDir, DiffractionCoeffs + froudeKrylovCoeffs);

    }

  }

  void HydrodynamicDataBase::ReadRadiation(const HighFive::File &HDF5_file, const std::string &path, Body* body) {

    auto radiationPath = path + "/Radiation";

    auto motionMaskMatrix = body->GetMotionMask().GetMatrix();

    auto nbTimeSamples = m_timeDiscretization.GetNbSample();

    for (unsigned int ibodyMotion = 0; ibodyMotion < m_nbody; ++ibodyMotion) {

      auto bodyMotion = this->GetBody(ibodyMotion);
      auto bodyMotionPath = radiationPath + "/BodyMotion_" + std::to_string(ibodyMotion);

      // Reading the infinite added mass matrix for the body.
      auto infiniteAddedMass = H5Easy::load<Eigen::MatrixXd>(HDF5_file, bodyMotionPath + "/InfiniteAddedMass");
      body->SetInfiniteAddedMass(bodyMotion, infiniteAddedMass);



    }


  }
} // namespace HDB5_io