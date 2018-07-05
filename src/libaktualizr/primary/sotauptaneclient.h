#include <gtest/gtest.h>
#include <json/json.h>
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bootloader/bootloader.h"
#include "commands.h"
#include "config/config.h"
#include "events.h"
#include "http/httpclient.h"
#include "package_manager/packagemanagerinterface.h"
#include "reportqueue.h"
#include "storage/invstorage.h"
#include "uptane/directorrepository.h"
#include "uptane/fetcher.h"
#include "uptane/imagesrepository.h"
#include "uptane/ipsecondarydiscovery.h"
#include "uptane/secondaryinterface.h"
#include "uptane/uptanerepository.h"

class SotaUptaneClient {
 public:
  SotaUptaneClient(Config &config_in, std::shared_ptr<event::Channel> events_channel_in,
                   Uptane::Manifest &uptane_manifest_in, std::shared_ptr<INvStorage> storage_in,
                   HttpInterface &http_client, const Bootloader &bootloader_in, ReportQueue &report_queue_in);

  bool initialize();
  bool updateMeta();
  bool uptaneIteration();
  bool uptaneOfflineIteration(std::vector<Uptane::Target> *targets);
  bool downloadImages(const std::vector<Uptane::Target> &targets);
  void runForever(const std::shared_ptr<command::Channel> &commands_channel);
  Json::Value AssembleManifest();
  std::string secondaryTreehubCredentials() const;
  Uptane::Exception getLastException() const { return last_exception; }

  // ecu_serial => secondary*
  std::map<Uptane::EcuSerial, std::shared_ptr<Uptane::SecondaryInterface> > secondaries;

 private:
  FRIEND_TEST(Uptane, offlineIteration);
  bool isInstalledOnPrimary(const Uptane::Target &target);
  std::vector<Uptane::Target> findForEcu(const std::vector<Uptane::Target> &targets, const Uptane::EcuSerial &ecu_id);
  data::InstallOutcome PackageInstall(const Uptane::Target &target);
  void PackageInstallSetResult(const Uptane::Target &target);
  void reportHwInfo();
  void reportInstalledPackages();
  void reportNetworkInfo();
  void initSecondaries();
  void verifySecondaries();
  void sendMetadataToEcus(const std::vector<Uptane::Target> &targets);
  void sendImagesToEcus(std::vector<Uptane::Target> targets);
  bool hasPendingUpdates(const Json::Value &manifests);
  void sendDownloadReport();
  bool putManifest();
  bool getNewTargets(std::vector<Uptane::Target> *new_targets);
  bool downloadTargets(const std::vector<Uptane::Target> &targets);
  void rotateSecondaryRoot(Uptane::RepositoryType repo, Uptane::SecondaryInterface &secondary);
  bool updateDirectorMeta();
  bool updateImagesMeta();
  bool checkImagesMetaOffline();
  bool checkDirectorMetaOffline();

  Config &config;
  std::shared_ptr<event::Channel> events_channel;
  Uptane::DirectorRepository director_repo;
  Uptane::ImagesRepository images_repo;
  Uptane::Manifest &uptane_manifest;
  std::shared_ptr<INvStorage> storage;
  std::shared_ptr<PackageManagerInterface> pacman;
  HttpInterface &http;
  Uptane::Fetcher uptane_fetcher;
  const Bootloader &bootloader;
  ReportQueue &report_queue;
  Json::Value operation_result;
  std::atomic<bool> shutdown = {false};
  Json::Value last_network_info_reported;
  std::map<Uptane::EcuSerial, Uptane::HardwareIdentifier> hw_ids;
  std::map<Uptane::EcuSerial, std::string> installed_images;

  Uptane::Exception last_exception{"", ""};
};

class SerialCompare {
 public:
  explicit SerialCompare(Uptane::EcuSerial target_in) : target(std::move(target_in)) {}
  bool operator()(std::pair<Uptane::EcuSerial, Uptane::HardwareIdentifier> &in) { return (in.first == target); }

 private:
  Uptane::EcuSerial target;
};
